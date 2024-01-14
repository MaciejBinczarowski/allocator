import subprocess
import unittest

class e2eTests(unittest.TestCase):
    
    def test_correct_scenario(self):
        result = subprocess.run(['./build/test/e2e/correct_scenario'], capture_output=True, text=True)
        stdout = result.stdout
        stderr = result.stderr

        # Checks wether program was correct
        self.assertEqual(result.returncode, 0, "Test not passed")

    def test_seq_fault(self):
        result = subprocess.run(['./build/test/e2e/seq_fault_test'], capture_output=True, text=True)

        # checks wether seq fault occured
        self.assertLess(result.returncode, 0, "Test not passed")

if __name__ == '__main__':
    unittest.main()