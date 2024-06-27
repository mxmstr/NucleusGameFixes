using Newtonsoft.Json;
using Org.BouncyCastle.Crypto.Engines;
using Org.BouncyCastle.Crypto.Modes;
using Org.BouncyCastle.Crypto.Parameters;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MGO3Test2
{
    internal class Program
    {
        static void Main(string[] args)
        {

            string base64String = "YnHdLj/1b4RBZXynl0xG1B0domEayp/1lLk99kX4wjo7NblxIkhv1ByeVvdNenjEJTavALlsfZfSgBpLKuCMvHkaMHdNOW9g+4ytGq/cFcXOpW6W3rjoDzBVAFXLVj+HRATx/hb68EX3+00fDqDfc0/wdXEaV+G7h5Zc4M2QoF5juLcqskL1iLDYQlLVsTH5VCgC7mK204ygBrK6BopI6RZN6pX+6R+lfT/01GExQVs=";
            //string base64String = "YnHdLj/1b4RBZXynl0xG1B0domEayp/1LMojq6tQ2JDRCuq4vFOnjrcmEnYyhX2g53Vz7aowKt+EVzqGW9ksvQeMWOeYWxuZaj9Kypfl33nW4xS+gkOu8ZIhsn3DDlyCvj4/ygqH0zbpRxitLa6vKmYTlYB8niSWz1DW+Mh+MZ95YAkibYdI6vLUYmIvQZgg337ElM9JYg4GdTtFCRAc8kAUdbf8FMrEi4Q4qrartezAnh+DddW6Eg==";
            //string base64String = "httpMsg=YnHdLj/1b4RBZXynl0xG1B0domEayp/17qTYbuN901HvdhaQvSeeICCU7YOuMYuVR0uBl/PGbviRe8JOsuj8jpC707k4KbiI8Rk/tOstpCalEie9u5mbdeFZD5QkAJKEalFGLYcPwauCoojZhYrj/tMfHQpKiEBpWt5jh0nj9GTTJyzYunBGcySyFTOfS3VRwuA0G941J6/bNDCDd4aB+QjhahQvngLyRo1gPGTTuZXgt7Bp5C29yMjPjO4/NeafEXD6vclxO0jEBc5ahU/hXCb2nvSwI8kPD2B3B+SviP1VoyhT7J0n/VWkvdm4xPV2kRFVp2fVtjTavq2eIZ7kxU/wdXEaV+G7h5Zc4M2QoF66D1yGFevDWPx648nUdnDaEWHtHetXB/ippguVlsIhqwe3K6Xw6FkHfXQ0J78SAmU=";
            base64String = base64String.Substring(8);

            byte[] cipherText = Convert.FromBase64String(base64String);

            // The key should be the one used for encryption
            //byte[] key = Encoding.UTF8.GetBytes("your-key-here");
            byte[] key = File.ReadAllBytes("C:\\Users\\lynch\\Documents\\mgsv_emulator-2\\files\\static_key.bin");

            int i = 0;
            StringBuilder decryptedString = new StringBuilder();

            do//for (int i = 0; i < cipherText.Length; i += blockSize)
            {
                try
                {
                    BlowfishEngine engine = new BlowfishEngine();
                    CbcBlockCipher cipher = new CbcBlockCipher(engine);
                    cipher.Init(false, new KeyParameter(key));

                    int blockSize = cipher.GetBlockSize();

                    byte[] plainText = new byte[blockSize];
                    cipher.ProcessBlock(cipherText, i, plainText, 0);
                    decryptedString.Append(Encoding.UTF8.GetString(plainText));
                    //string decryptedString = Encoding.UTF8.GetString(plainText);
                    //Console.WriteLine(decryptedString);

                    i += blockSize;
                }
                catch (Exception e)
                {
                    Console.WriteLine(e.Message);
                    break;
                }
            }
            while (true);

            // Remove padding
            int padding = decryptedString.ToString().LastIndexOf('}');
            decryptedString.Remove(padding + 1, decryptedString.Length - padding - 1);

            Console.WriteLine(decryptedString.ToString());

            // Convert decrypted string to JSON
            dynamic data = JsonConvert.DeserializeObject(decryptedString.ToString());
            Console.WriteLine(data["data"]);

        }
    }
}
