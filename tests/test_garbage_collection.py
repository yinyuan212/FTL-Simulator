import unittest
from .ftl_driver import FTLDriver, NAND_SUCCESS

class TestGarbageCollection(unittest.TestCase):
    """
    垃圾回收測試 (Garbage Collection Test Suite)
    藉由大量的覆寫觸發空間不足，強迫底層啟動 GC 重整。
    """
    
    def setUp(self):
        self.driver = FTLDriver()
        self.driver.initialize()

    def tearDown(self):
        self.driver.cleanup()

    def test_gc_triggering_and_data_integrity(self):
        """
        測試：透過在有限的 LPA 範圍內進行幾千次寫入，
        產生大量 Invalid Pages，強迫系統執行 Garbage Collection。
        最後驗證這些位址上最新寫入的資料是否依然正確。
        """
        NUM_WRITES = 3000   # 寫入總量，視 NAND 總容量而定，需大到足以觸發 GC
        LBA_POOL_SIZE = 50  # 邏輯區塊池
        
        # 產生大量覆寫，創造 Invalid 頁面
        for i in range(NUM_WRITES):
            lpa = i % LBA_POOL_SIZE
            data = f"GC_Test_Data_Seq_{i}".encode('utf-8')
            
            status = self.driver.write_page(lpa, data)
            self.assertEqual(status, NAND_SUCCESS, f"Write failed at sequence {i}")
            
        # 歷體驗證：所有的 LBA_POOL_SIZE 最新資料是否正確
        for lpa in range(LBA_POOL_SIZE):
            status, read_data = self.driver.read_page(lpa)
            self.assertEqual(status, NAND_SUCCESS)
            
            # 計算該 LPA 最後一次被寫入時的 Sequence Number
            # 例如 NUM_WRITES = 3000, LBA_POOL_SIZE = 50，那 lpa=0 最後一次寫入是 i=2950
            last_i = NUM_WRITES - LBA_POOL_SIZE + lpa
            expected_data = f"GC_Test_Data_Seq_{last_i}".encode('utf-8')
            clean_data = read_data.rstrip(b'\x00')
            
            self.assertEqual(clean_data, expected_data, f"Data corrupted at LPA {lpa} after GC operations")

if __name__ == '__main__':
    unittest.main()
