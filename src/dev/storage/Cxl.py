from m5.SimObject import SimObject
from m5.params import *
from m5.objects.PciDevice import *

class CxlMemory(SimObject):
    type = 'CxlMemory'
    cxx_header = "dev/storage/cxl_controller.hh"
    cxx_class = 'gem5::CxlMemory'

    size = Param.MemorySize('64MiB', "Size of the Cxl Memory")

class CxlController(PciDevice):
    type = 'CxlController'
    cxx_header = "dev/storage/cxl_controller.hh"
    cxx_class = 'gem5::CxlController'

    latency = Param.Latency(
        '50ns', "cxl-controller device's latency for mem access")
    cxl_mem_latency = Param.Latency(
        '2ns', "cxl.mem protocol processing's latency for device")
    memories = VectorParam.CxlMemory("Cxl Memories attached to this controller")

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
