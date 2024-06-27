using System;
using System.IO;
using System.Text;
using System.Collections.Generic;
using System.Security.Cryptography;
using System.Runtime.InteropServices;
using Newtonsoft.Json;
using Ionic.Zlib;

namespace MGO3Test2
{
    public class Decoder
    {
        public Blowfish __static_blowfish;
        private Blowfish __session_blowfish;
        private byte[] __crypto_key;

        private const string DECODE_PACK = ">l";

        public Decoder(byte[] static_key = null, byte[] crypto_key = null)
        {
            __static_blowfish = new Blowfish();
            if (static_key == null)
            {
                static_key = 
                    
                    File.ReadAllBytes("C:\\Users\\lynch\\Documents\\mgsv_emulator-2\\files\\static_key.bin");
            }
            __static_blowfish.initialize(static_key);

            __session_blowfish = null;
            __crypto_key = null;

            if (crypto_key != null)
            {
                __crypto_key = crypto_key;
                __init_session_blowfish();
            }
        }

        private byte[] __get_crypto_key(dynamic data)
        {
            byte[] crypto_key = null;
            if (data.ContainsKey("data"))
            {
                if (data["data"] is Dictionary<string, dynamic>)
                {
                    if (data["data"].ContainsKey("crypto_key"))
                    {
                        if (data["data"]["crypto_key"].Length > 0)
                        {
                            crypto_key = Convert.FromBase64String(data["data"]["crypto_key"]);
                            __crypto_key = crypto_key;
                        }
                    }
                }
            }
            return crypto_key;
        }

        private void __init_session_blowfish(byte[] crypto_key = null)
        {
            __session_blowfish = new Blowfish();
            if (crypto_key != null)
            {
                __crypto_key = crypto_key;
            }
            __session_blowfish.initialize(__crypto_key);
        }

        private dynamic __get_json(string text)
        {
            text = text.Substring(0, text.LastIndexOf('}') + 1);
            text = text.Replace("\\\\r\\\\n", "");
            text = text.Replace("\\r\\n", "");
            text = text.Replace("\\", "");
            text = text.Replace("\"{", "{");
            text = text.Replace("}\"", "}");
            return JsonConvert.DeserializeObject(text);
        }

        public byte[] __decipher(Blowfish blow, byte[] data)
        {
            int offset = 0;
            List<byte> full_text = new List<byte>();
            while (offset != data.Length)
            {
                byte[] chunk = new byte[8];
                Array.Copy(data, offset, chunk, 0, 8);
                uint x = (uint)BitConverter.ToInt32(chunk, 0);
                uint y = (uint)BitConverter.ToInt32(chunk, 4);
                blow.blowfish_decipher(ref x, ref y);

                byte[] x_text = BitConverter.GetBytes(x);
                byte[] y_text = BitConverter.GetBytes(y);

                full_text.AddRange(x_text);
                full_text.AddRange(y_text);
                offset += 8;
            }
            return full_text.ToArray();
        }

        public dynamic Decode(string data)
        {
            byte[] data_encoded = Convert.FromBase64String(data);
            byte[] data_decoded = __decipher(__static_blowfish, data_encoded);

            string decoded_data = Encoding.UTF8.GetString(data_decoded);
            dynamic data_json = __get_json(decoded_data);

            if (__session_blowfish == null)
            {
                __get_crypto_key(data_json);
                if (__crypto_key != null)
                {
                    __init_session_blowfish();
                }
            }

            if (data_json["session_crypto"])
            {
                if (__session_blowfish != null)
                {
                    byte[] embedded = Convert.FromBase64String(data_json["data"]);
                    data_json["data"] = __decipher(__session_blowfish, embedded);
                    if (data_json["compress"])
                    {
                        data_json["data"] = ZlibStream.UncompressBuffer(data_json["data"]);
                    }
                }
                else
                {
                    // encryption is used, but we have no session key
                    // this is ok, since we need to get enc key from mysql using session id
                }
            }
            else
            {
                // no encryption, used in CMD_GET_URLLIST and others before getting session key
                if (data_json["compress"])
                {
                    data_json["data"] = ZlibStream.UncompressBuffer(Convert.FromBase64String(data_json["data"]));
                }
            }

            if (data_json["data"] is byte[])
            {
                data_json["data"] = Encoding.UTF8.GetString(data_json["data"]);
            }

            if (data_json.ContainsKey("original_size") && data_json["data"] is string)
            {
                data_json["data"] = data_json["data"].Substring(0, data_json["original_size"]);
                try
                {
                    dynamic j = JsonConvert.DeserializeObject(data_json["data"]);
                    data_json["data"] = j;
                }
                catch
                {
                    // not json, skipping
                }
            }
            return data_json;
        }
    }
}
