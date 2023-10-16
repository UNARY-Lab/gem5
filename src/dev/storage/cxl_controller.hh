#ifndef __DEV_STORAGE_CXL_CTRL_HH__
#define __DEV_STORAGE_CXL_CTRL_HH__

#include "base/addr_range.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "dev/pci/device.hh"
#include "dev/reg_bank.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "params/CxlController.hh"
#include "params/CxlMemory.hh"

namespace gem5 {

class CxlController : public PciDevice {
private:
  // Below is taken from ide_ctrl.hh
  /** Registers used in device specific PCI configuration */
  class ConfigSpaceRegs : public RegisterBankLE {
  public:
    ConfigSpaceRegs(const std::string &name)
        : RegisterBankLE(name, PCI_DEVICE_SPECIFIC) {
      // None of these registers are actually hooked up to control
      // anything, so they have no specially defined behaviors. They
      // just store values for now, but should presumably do something
      // in a more accurate model.
      addRegisters({primaryTiming, secondaryTiming, deviceTiming, raz0,
                    udmaControl, raz1, udmaTiming, raz2});
    }

    enum { TimeRegWithDecodeEnabled = 0x8000 };

    /* Offset in config space */
    /* 0x40-0x41 */ Register16 primaryTiming = {"primary timing",
                                                TimeRegWithDecodeEnabled};
    /* 0x42-0x43 */ Register16 secondaryTiming = {"secondary timing",
                                                  TimeRegWithDecodeEnabled};
    /* 0x44      */ Register8 deviceTiming = {"device timing"};
    /* 0x45-0x47 */ RegisterRaz raz0 = {"raz0", 3};
    /* 0x48      */ Register8 udmaControl = {"udma control"};
    /* 0x49      */ RegisterRaz raz1 = {"raz1", 1};
    /* 0x4a-0x4b */ Register16 udmaTiming = {"udma timing"};
    /* 0x4c-...  */ RegisterRaz raz2 = {"raz2", PCI_CONFIG_SIZE - 0x4c};

    void serialize(CheckpointOut &cp) const;
    void unserialize(CheckpointIn &cp);
  };

  ConfigSpaceRegs configSpaceRegs;

  Tick latency_;
  Tick cxl_mem_latency_;

  std::vector<CxlMemory *> cxl_mem_;

public:
  PARAMS(CxlController);
  virtual Tick read(PacketPtr pkt) override;
  virtual Tick write(PacketPtr pkt) override;

  virtual AddrRangeList getAddrRanges() const override;

  Tick resolve_cxl_mem(PacketPtr ptk);
  Tick writeConfig(PacketPtr pkt);
  Tick readConfig(PacketPtr pkt);

  CxlController(const Params &p);
};

class CxlMemory : public SimObject {
private:
  uint64_t size_;
  uint8_t *memory;

public:
  PARAMS(CxlMemory);
  void access(PacketPtr pkt, Addr addr, int bar_num, Addr offset);

  CxlMemory(const CxlMemory &other) = delete;
  CxlMemory &operator=(const CxlMemory &other) = delete;

  uint64_t size() const { return size_; }

  ~CxlMemory() { delete memory; }
  CxlMemory(const Params &p);
};

} // namespace gem5

#endif // __DEV_STORAGE_CXL_CTRL_HH__
