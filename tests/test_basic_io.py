import unittest
from .ftl_driver import FTLDriver, NAND_SUCCESS

class TestBasicIO(unittest.TestCase):
    """
    基礎讀寫測試套件 (Basic IO Test Suite)
    負責驗證單一頁面或少量頁面的正確性。
    """
    
    def setUp(self):
        # 每個測試案例開始前，初始化 FTL 環境
        self.driver = FTLDriver()
        self.driver.initialize()

    def tearDown(self):
        # 每個測試完畢後，清理記憶體
        self.driver.cleanup()

    def test_single_page_read_write(self):
        """測試：單一區塊寫入後讀取，驗證資料完整性"""
        lpa = 10
        test_data = b"Hello FTL Validation!"
        
        # 寫入 (Write)
        status = self.driver.write_page(lpa, test_data)
        self.assertEqual(status, NAND_SUCCESS, f"Write failed with status {status}")
        
        # 讀取 (Read)
        status, read_data = self.driver.read_page(lpa)
        self.assertEqual(status, NAND_SUCCESS, f"Read failed with status {status}")
        
        # 驗證 (Verify)
        clean_data = read_data.rstrip(b'\x00')
        self.assertEqual(clean_data, test_data, "Data mismatch on single page read/write")

    def test_overwrite_page(self):
        """測試：對同一個邏輯位址(LPA)重複寫入，驗證舊資料是否被覆蓋"""
        lpa = 42
        data_v1 = b"Data Version 1"
        data_v2 = b"Data Version 2"
        
        # 首次寫入
        self.driver.write_page(lpa, data_v1)
        
        # 覆寫 (Overwrite)，這會讓底層 physical page 變成 Invalid，並映射到新 page
        self.driver.write_page(lpa, data_v2)
        
        # 讀取最新資料
        status, read_data = self.driver.read_page(lpa)
        self.assertEqual(status, NAND_SUCCESS)
        
        clean_data = read_data.rstrip(b'\x00')
        self.assertEqual(clean_data, data_v2, "Overwritten data validation failed")

if __name__ == '__main__':
    unittest.main()
