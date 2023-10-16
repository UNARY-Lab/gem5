#include "dev/storage/cxl_controller.hh"
#include "base/addr_range.hh"
#include "base/trace.hh"
#include "debug/CxlController.hh"
#include "debug/CxlMemory.hh"

namespace gem5 {

CxlController::CxlController(const Params &p)
    : PciDevice(p), configSpaceRegs(name() + ".config_space_regs"),
      latency_(p.latency), cxl_mem_latency_(p.cxl_mem_latency),
      cxl_mem_(params().memories) {}

Tick CxlController::read(PacketPtr pkt) {
  Tick cxl_latency = resolve_cxl_mem(pkt);
  Addr addr = pkt->getAddr();
  if (pkt->cacheResponding()) {
    DPRINTF(CxlController, "Cache responding to %#x: not responding\n", addr);
    return 0;
  }

  if (pkt->cmd == MemCmd::CleanEvict || pkt->cmd == MemCmd::WritebackClean) {
    DPRINTF(CxlController, "CleanEvict  on %#x: not responding\n", addr);
    return 0;
  }

  int bar_num;
  Addr offset;
  panic_if(!getBAR(addr, bar_num, offset),
           "CXL memory access to unmapped address %#x\n", addr);
  Addr cOffset = 0;
  for (auto &mem : cxl_mem_) {
    cOffset += mem->size();
    if (offset < cOffset) {
      mem->access(pkt, addr, bar_num, offset);
      break;
    }
  }
  return latency_ + cxl_latency;
}

Tick CxlController::write(PacketPtr pkt) {
  Tick cxl_latency = resolve_cxl_mem(pkt);
  Addr addr = pkt->getAddr();
  if (pkt->cacheResponding()) {
    DPRINTF(CxlController, "Cache responding to %#x: not responding\n", addr);
    return 0;
  }

  if (pkt->cmd == MemCmd::CleanEvict || pkt->cmd == MemCmd::WritebackClean) {
    DPRINTF(CxlController, "CleanEvict  on %#x: not responding\n", addr);
    return 0;
  }

  int bar_num;
  Addr offset;
  panic_if(!getBAR(addr, bar_num, offset),
           "CXL memory access to unmapped address %#x\n", addr);
  Addr cOffset = 0;
  for (auto &mem : cxl_mem_) {
    cOffset += mem->size();
    if (offset < cOffset) {
      mem->access(pkt, addr, bar_num, offset);
      break;
    }
  }
  return latency_ + cxl_latency;
}

AddrRangeList CxlController::getAddrRanges() const {
  AddrRangeList ranges = PciDevice::getAddrRanges();
  AddrRangeList ret_ranges;
  // ranges starting with 0 haven't been assigned
  for (const auto &r : ranges)
    if (r.start() != 0) {
      DPRINTF(CxlController, "adding range: %s\n", r.to_string());
      ret_ranges.push_back(r);
    }
  return ret_ranges;
}

Tick CxlController::resolve_cxl_mem(PacketPtr pkt) {
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

CxlMemory::CxlMemory(const Params &p)
    : SimObject(p), size_(p.size), memory(new uint8_t[size_]) {}

Tick CxlController::readConfig(PacketPtr pkt) {
  int offset = pkt->getAddr() & PCI_CONFIG_SIZE;
  if (offset < PCI_DEVICE_SPECIFIC)
    return PciDevice::readConfig(pkt);

  size_t size = pkt->getSize();

  configSpaceRegs.read(offset, pkt->getPtr<void>(), size);

  DPRINTF(CxlController,
          "PCI Config read offset: %#x size: %d data: %#x addr: %#x\n", offset,
          size, pkt->getUintX(ByteOrder::little), pkt->getAddr());

  pkt->makeAtomicResponse();
  return latency_ + resolve_cxl_mem(pkt);
}

Tick CxlController::writeConfig(PacketPtr pkt) {
  int offset = pkt->getAddr() & PCI_CONFIG_SIZE;
  if (offset < PCI_DEVICE_SPECIFIC)
    return PciDevice::writeConfig(pkt);

  size_t size = pkt->getSize();

  DPRINTF(CxlController, "PCI Config write offset: %#x size: %d data: %#x\n",
          offset, size, pkt->getUintX(ByteOrder::little));
  configSpaceRegs.write(offset, pkt->getConstPtr<void>(), size);

  pkt->makeAtomicResponse();
  return latency_ + resolve_cxl_mem(pkt);
}

void CxlMemory::access(PacketPtr pkt, Addr addr, int bar_num, Addr offset) {
  // Below is borrowed from abstract_mem.cc
  uint8_t *host_addr = (uint8_t *)(offset + memory);

  // TODO: add statistics (from abstract_mem)
  if (pkt->isRead()) {
    DPRINTF(CxlMemory, "Read at addr %#x, size %d\n", addr, pkt->getSize());
    pkt->setData(host_addr);
    pkt->makeResponse();
  } else if (pkt->isWrite()) {
    DPRINTF(CxlMemory, "Write at addr %#x, size %d\n", addr,
            pkt->getSize());
    pkt->writeData(host_addr);
    pkt->makeResponse();
  } else
    panic("AbstractMemory: unimplemented functional command %s",
          pkt->cmdString());
}
} // namespace gem5
