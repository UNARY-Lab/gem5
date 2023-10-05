#include "base/addr_range.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "dev/pci/device.hh"
#include "dev/reg_bank.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"
#include "params/CxlMemory.hh"

namespace gem5 {

class CxlMemory : public PciDevice {
private:
  class Memory {
  private:
    AddrRange range;
    CxlMemory *cxl_root;
    uint8_t *memory;
    const std::string name_ = "CxlMemory::Memory";

  public:
    Memory(const AddrRange &range, CxlMemory *cxl_root);
    const std::string &name() const { return name_; }
    void access(PacketPtr pkt);

    Memory(const Memory &other) = delete;
    Memory &operator=(const Memory &other) = delete;

    ~Memory() { delete memory; }
  };

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

  AddrRange addr_range_;
  Memory mem_;

  Tick latency_;
  Tick cxl_mem_latency_;

public:
  virtual Tick read(PacketPtr pkt) override;
  virtual Tick write(PacketPtr pkt) override;

  virtual AddrRangeList getAddrRanges() const override;

  Tick resolve_cxl_mem(PacketPtr ptk);
  Tick writeConfig(PacketPtr pkt);
  Tick readConfig(PacketPtr pkt);

  using Param = CxlMemoryParams;
  CxlMemory(const Param &p);
};

} // namespace gem5
