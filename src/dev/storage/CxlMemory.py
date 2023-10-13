from m5.SimObject import SimObject
from m5.params import *
from m5.objects.PciDevice import *


class CxlMemory(PciDevice):
    type = 'CxlMemory'
    cxx_header = "dev/storage/cxl_memory.hh"
    cxx_class = 'gem5::CxlMemory'

    latency = Param.Latency(
        '50ns', "cxl-memory device's latency for mem access")
    cxl_mem_latency = Param.Latency(
        '2ns', "cxl.mem protocol processing's latency for device")

    VendorID = 0x6969 # random number
    DeviceID = 0x0420 # random number
    Command = 0x2 # set memory bit
    Status = 0x80 # set fast back-to-back bit ? not sure what it does
    Revision = 0x0
    ClassCode = 0xFF # Unassigned class
    SubClassCode = 0x00 # N/A
    ProgIF = 0x00 # N/A
    InterruptLine = 0xFF # No connection
    InterruptPin = 0x00 # No interrupt pin

    # Primary
    BAR0 = PciMemBar(size='128MiB')
    BAR1 = PciMemUpperBar()
