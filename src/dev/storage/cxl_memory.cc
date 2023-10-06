#include "dev/storage/cxl_memory.hh"
#include "base/addr_range.hh"
#include "base/trace.hh"
#include "debug/CxlMemory.hh"

namespace gem5 {

CxlMemory::CxlMemory(const Param &p)
    : PciDevice(p), configSpaceRegs(name() + ".config_space_regs"),
      addr_range_(RangeSize(p.BAR0->addr(), p.BAR0->size())),
      mem_(addr_range_, this), latency_(p.latency),
      cxl_mem_latency_(p.cxl_mem_latency) {}

Tick CxlMemory::read(PacketPtr pkt) {
  Tick cxl_latency = resolve_cxl_mem(pkt);
  mem_.access(pkt);
  return latency_ + cxl_latency;
}

Tick CxlMemory::write(PacketPtr pkt) {
  Tick cxl_latency = resolve_cxl_mem(pkt);
  mem_.access(pkt);
  return latency_ + cxl_latency;
}

AddrRangeList CxlMemory::getAddrRanges() const {
  AddrRangeList ranges = PciDevice::getAddrRanges();
  AddrRangeList ret_ranges;
  ret_ranges.push_back(addr_range_);
  // ranges starting with 0 haven't been assigned
  for (const auto &r : ranges)
    if (r.start() != 0) {
      DPRINTF(CxlMemory, "adding range: %s\n", r.to_string());
      ret_ranges.push_back(r);
    }
  return ret_ranges;
}

Tick CxlMemory::resolve_cxl_mem(PacketPtr pkt) {
  // TODO: add ability to have a topology of CXL memory
  if (pkt->cmd == MemCmd::ReadReq) {
    assert(pkt->isRead());
    assert(pkt->needsResponse());
  } else if (pkt->cmd == MemCmd::WriteReq) {
    assert(pkt->isWrite());
    assert(pkt->needsResponse());
  }
  return cxl_mem_latency_;
}

CxlMemory::Memory::Memory(const AddrRange &range, CxlMemory *cxl_root)
    : range(range), cxl_root(cxl_root), memory(new uint8_t[range.size()]) {}

Tick CxlMemory::readConfig(PacketPtr pkt) {
  int offset = pkt->getAddr() & PCI_CONFIG_SIZE;
  if (offset < PCI_DEVICE_SPECIFIC)
    return PciDevice::readConfig(pkt);

  size_t size = pkt->getSize();

  configSpaceRegs.read(offset, pkt->getPtr<void>(), size);

  DPRINTF(CxlMemory,
          "PCI Config read offset: %#x size: %d data: %#x addr: %#x\n", offset,
          size, pkt->getUintX(ByteOrder::little), pkt->getAddr());

  pkt->makeAtomicResponse();
  return latency_ + resolve_cxl_mem(pkt);
}

Tick CxlMemory::writeConfig(PacketPtr pkt) {
  int offset = pkt->getAddr() & PCI_CONFIG_SIZE;
  if (offset < PCI_DEVICE_SPECIFIC)
    return PciDevice::writeConfig(pkt);

  size_t size = pkt->getSize();

  DPRINTF(CxlMemory, "PCI Config write offset: %#x size: %d data: %#x\n",
          offset, size, pkt->getUintX(ByteOrder::little));
  configSpaceRegs.write(offset, pkt->getConstPtr<void>(), size);

  pkt->makeAtomicResponse();
  return latency_ + resolve_cxl_mem(pkt);
}

void CxlMemory::Memory::access(PacketPtr pkt) {
  // Below is borrowed from abstract_mem.cc
  Addr addr = pkt->getAddr();
  if (pkt->cacheResponding()) {
    DPRINTF(CxlMemory, "Cache responding to %#x: not responding\n", addr);
    return;
  }

  if (pkt->cmd == MemCmd::CleanEvict || pkt->cmd == MemCmd::WritebackClean) {
    DPRINTF(CxlMemory, "CleanEvict  on %#x: not responding\n", addr);
    return;
  }

  int bar_num;
  Addr offset;
  panic_if(!cxl_root->getBAR(addr, bar_num, offset),
           "CXL memory access to unmapped address %#x\n", addr);

  uint8_t *host_addr = (uint8_t *)(offset + memory);

  // TODO: add statistics (from abstract_mem)
  if (pkt->isRead()) {
    DPRINTF(CxlMemory, "Read at addr %#x, size %d\n", addr, pkt->getSize());
    pkt->setData(host_addr);
    pkt->makeResponse();
  } else if (pkt->isWrite()) {
    DPRINTF(CxlMemory, "Write at addr %#x, size %d\n", addr, pkt->getSize());
    pkt->writeData(host_addr);
    pkt->makeResponse();
  } else
    panic("AbstractMemory: unimplemented functional command %s",
          pkt->cmdString());
}
} // namespace gem5
