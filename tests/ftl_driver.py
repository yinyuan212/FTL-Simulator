import ctypes
import os

NAND_BYTES_PER_PAGE = 4096
NAND_SUCCESS = 0

class FTLDriver:
    """
    這個類別負責與 C 語言編譯出來的 DLL 溝通，扮演硬體抽象層/驅動程式的角色。
    """
    def __init__(self, dll_path=None):
        if dll_path is None:
            # 預設會往上一層目錄尋找 build/ftl_simulator.dll
            script_dir = os.path.dirname(os.path.abspath(__file__))
            dll_path = os.path.join(script_dir, "..", "build", "ftl_simulator.dll")
            
        self.ftl_lib = ctypes.CDLL(os.path.abspath(dll_path))
        
        # 設定 C 函式的參數型態 (argtypes) 與回傳型態 (restype)
        self.ftl_lib.nand_init.argtypes = []
        self.ftl_lib.ftl_init.argtypes = []
        self.ftl_lib.gc_init.argtypes = []
        
        self.ftl_lib.ftl_write.argtypes = [ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint8)]
        self.ftl_lib.ftl_write.restype = ctypes.c_int
        
        self.ftl_lib.ftl_read.argtypes = [ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint8)]
        self.ftl_lib.ftl_read.restype = ctypes.c_int
        
        self.ftl_lib.ftl_cleanup.argtypes = []
        self.ftl_lib.nand_cleanup.argtypes = []

    def initialize(self):
        self.ftl_lib.nand_init()
        self.ftl_lib.ftl_init()
        self.ftl_lib.gc_init()

    def write_page(self, lpa, data_bytes):
        if len(data_bytes) > NAND_BYTES_PER_PAGE:
            raise ValueError("Data too large for one page")
            
        # 補齊空位元組到一個 Page 的長度
        padded_data = bytearray(data_bytes) + b'\x00' * (NAND_BYTES_PER_PAGE - len(data_bytes))
        DataArray = ctypes.c_uint8 * NAND_BYTES_PER_PAGE
        c_data = DataArray.from_buffer(padded_data)
        
        return self.ftl_lib.ftl_write(lpa, c_data)
        
    def read_page(self, lpa):
        DataArray = ctypes.c_uint8 * NAND_BYTES_PER_PAGE
        c_data = DataArray()
        
        status = self.ftl_lib.ftl_read(lpa, c_data)
        if status != NAND_SUCCESS:
            return status, None
            
        return status, bytes(c_data)

    def cleanup(self):
        self.ftl_lib.ftl_cleanup()
        self.ftl_lib.nand_cleanup()
