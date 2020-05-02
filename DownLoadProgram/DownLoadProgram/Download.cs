using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using UsbHid;
using UsbHid.USB.Classes.Messaging;
using System.Security.Cryptography;

namespace DownLoadProgram
{
    public partial class Download : Form
    {
        public UsbHidDevice Device;
        private SynchronizationContext _syncContext = null;
        AseCrypt aseCrypt = new AseCrypt();
        public Download()
        {
            InitializeComponent();
            //
            SetupHid();
            _syncContext = WindowsFormsSynchronizationContext.Current;
        }
        //
        private void SetupHid()
        {
            Device = new UsbHidDevice(5566, 22352);
            if (Device.Connect())
            {
                Device.OnConnected += DeviceOnConnected;
                Device.OnDisConnected += DeviceOnDisConnected;
                Device.DataReceived += DeviceDataReceived;
            }
            else
            {
                MessageBox.Show("Canot connect to control Box, Please check usb cable!");
            }
        }

        private void DeviceDataReceived(byte[] data)
        {
            UpdateTextDebug(data);
        }

        private void DeviceOnDisConnected()
        {
        }

        private void DeviceOnConnected()
        {
        }

        private void btnDownLoad_Click(object sender, EventArgs e)
        {
            UploadBinFile(textBox1.Text, 48);
            MessageBox.Show("Message Complete");
        }
        /// <summary>
        /// Truyen du lieu sang thiet bi
        /// </summary>
        /// <param name="BinFilePath">Duong dan den file bin</param>
        /// <param name="MaxLen">So byte lon nhan trong 1 lan truyen</param>
        private void UploadBinFile(string BinFilePath, uint MaxLen)
        {
            // Read the file into <bits>
            var fs = new FileStream(@BinFilePath, FileMode.Open);
            var len = (int)fs.Length;
            var bits = new byte[len];
            fs.Read(bits, 0, len);
            fs.Close();
            uint Datalength = 0;
            // Dump 16 bytes per line
            for (uint ix = 0; ix < len; ix += MaxLen)
            {
                Datalength = Math.Min((uint)MaxLen,(uint)len - ix);
                //
                if(ix >= 0xbd0)
                {
                    int s = 0;
                    s = 1;
                }
                byte[] DataSend = new byte[Datalength];
                Array.Copy(bits, ix, DataSend, 0, Datalength);
                SendToBoard(0x02, (byte)Datalength, ix, DataSend);
                Thread.Sleep(50);
            }
            // THONG BAO TIEN TRINH KET THUC. NHAP THONG TIN FIRMWARE SAN SANG
            SendToBoard(0x05, (byte)8, 0, new byte[] { 0, 0, 0, 0, 0, 0, 0, 0 });
        }
        //
        public void SendToBoard(byte MsgID, byte MsgSize, UInt32 Post, byte[] DataSend)
        {
            if (!Device.IsDeviceConnected) return;
            byte[] mysend = new byte[63];
            byte[] tmpByte = new byte[4];
            //
            mysend[0] = (byte)('?');
            mysend[1] = (byte)('#');
            mysend[2] = (byte)('#');
            //
            mysend[3] = MsgID; // message ID
            mysend[4] = MsgSize; // message size
            //
            tmpByte = BitConverter.GetBytes(Post);
            mysend[5] = tmpByte[3];
            mysend[6] = tmpByte[2];
            mysend[7] = tmpByte[1];
            mysend[8] = tmpByte[0];
            //
            Array.Copy(DataSend,0, mysend,9, MsgSize);
            for (int ir = MsgSize + 9; ir < 63; ir++)
            {
                mysend[ir] = (byte)(0);
            }
            //
            var command = new CommandMessage(64/*So byte truyen di*/, mysend);
            Device.SendMessage(command);
        }
        //
        public void SendPassword(byte MsgID, byte MsgSize, byte[] DataSend)
        {
            if (!Device.IsDeviceConnected) return;
            byte[] mysend = new byte[63];
            byte[] tmpByte = new byte[4];
            //
            mysend[0] = (byte)('?');
            mysend[1] = (byte)('#');
            mysend[2] = (byte)('#');
            //
            mysend[3] = MsgID; // message ID
            mysend[4] = MsgSize; // message size
            //
            //
            Array.Copy(DataSend, 0, mysend, 5, MsgSize);
            for (int ir = MsgSize + 5; ir < 63; ir++)
            {
                mysend[ir] = (byte)(0);
            }
            //
            var command = new CommandMessage(64/*So byte truyen di*/, mysend);
            Device.SendMessage(command);
        }
        public void SendUsingDate(byte MsgID, DateTime TimeUsing)
        {
            if (!Device.IsDeviceConnected) return;
            byte[] mysend = new byte[63];
            byte[] tmpByte = new byte[8];
            //
            mysend[0] = (byte)('?');
            mysend[1] = (byte)('#');
            mysend[2] = (byte)('#');
            //
            mysend[3] = MsgID; // message ID
            mysend[4] = 8; // message size
            //
            DateTime TimeOfset = new DateTime(1970, 1, 1);
            TimeSpan MyTime = TimeUsing - TimeOfset;
            //
            UInt64 TotalSecond = (UInt64)MyTime.TotalSeconds;
            tmpByte = BitConverter.GetBytes(TotalSecond);
            mysend[5] = tmpByte[0];
            mysend[6] = tmpByte[1];
            mysend[7] = tmpByte[2];
            mysend[8] = tmpByte[3];
            mysend[9] = tmpByte[4];
            mysend[10] = tmpByte[5];
            mysend[11] = tmpByte[6];
            mysend[12] = tmpByte[7];
            //
            for (int ir = 8 + 5; ir < 63; ir++)
            {
                mysend[ir] = (byte)(0);
            }
            //
            var command = new CommandMessage(64/*So byte truyen di*/, mysend);
            Device.SendMessage(command);
        }

        private void Download_Load(object sender, EventArgs e)
        {
            textBox2.Text = ComputeSha256Hash("thiencuong123").ToUpper();
            textBox3.Text = ComputeSha256Hash("0123456799");
            //
            byte[] Key =  {
                            0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                            0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
                            0x76, 0x2e, 0x71, 0x60, 0xf3, 0x8b, 0x4d, 0xa5,
                            0x6a, 0x78, 0x4d, 0x90, 0x45, 0x19, 0x0c, 0xfe
                        };
            byte[] Input = // hex digits of pi
                            {
                                0x32, 0x43, 0xf6, 0xa8, 0x88, 0x5a, 0x30, 0x8d,
                                0x31, 0x31, 0x98, 0xa2, 0xe0, 0x37, 0x07, 0x34
                                //0x4a, 0x40, 0x93, 0x82, 0x22, 0x99, 0xf3, 0x1d,
                                //0x00, 0x82, 0xef, 0xa9, 0x8e, 0xc4, 0xe6, 0xc8
                            };
            //
            byte[] Decrpt = { 26, 110, 108, 44, 102, 46, 125, 166, 80, 31, 251, 98, 188, 158, 147, 243 };
            AseCrypt aesUser = new AseCrypt();
            byte[] myEncrypt = aesUser.AES_Encrypt(Input, Key);
            byte[] myDecrypt = aesUser.AES_Decrypt(Decrpt, Key);

        }
        private string ComputeSha256Hash(string rawData)
        {
            // Create a SHA256   
            using (SHA256 sha256Hash = SHA256.Create())
            {
                // ComputeHash - returns byte array  
                byte[] bytes = sha256Hash.ComputeHash(Encoding.UTF8.GetBytes(rawData));

                // Convert byte array to a string   
                StringBuilder builder = new StringBuilder();
                for (int i = 0; i < bytes.Length; i++)
                {
                    builder.Append(bytes[i].ToString("x2"));
                }
                return builder.ToString();
            }
        }
        //
        private void UpdateTextDebug(byte[] DataUpdate)
        {
            _syncContext.Post(delegate
            {
                //hidData.AppendText(ByteArrayToString(DataUpdate) + "\r\n"); 
                DataUpdate[0] = (byte)'@';
                hidData.AppendText(System.Text.Encoding.UTF8.GetString(DataUpdate) + "\r\n");
                label2.Text = DataUpdate.Length.ToString();
            }, null);
        }
        //
        private static string ByteArrayToString(ICollection<byte> input)
        {
            var result = string.Empty;

            if (input != null && input.Count > 0)
            {
                var isFirst = true;
                foreach (var b in input)
                {
                    result += isFirst ? string.Empty : " - ";
                    result += b.ToString("X2");
                    isFirst = false;
                }
            }
            return (result);
        }

        private void button1_Click(object sender, EventArgs e)
        {
            SendToBoard(0x01, (byte)7, 0, new byte[] {0,0,0,0,0,0,0 });
        }

        private void button2_Click(object sender, EventArgs e)
        {
            SendToBoard(0x03, (byte)8, 0, new byte[] { 0, 0, 0, 0, 0, 0, 0, 0 });
        }

        private void button3_Click(object sender, EventArgs e)
        {
            SendToBoard(0x04, (byte)8, 0, new byte[] { 0, 0, 0, 0, 0, 0, 0, 0 });
        }

        private void button4_Click(object sender, EventArgs e)
        {
            SendToBoard(0x05, (byte)8, 0, new byte[] { 0, 0, 0, 0, 0, 0, 0, 0 });
        }

        private void btnSetPass_Click(object sender, EventArgs e)
        {
            string Pass = "thiencuong123";
            SendPassword(0xA0, (byte)Pass.Length, Encoding.ASCII.GetBytes(Pass));
        }

        private void btnPasswordInput_Click(object sender, EventArgs e)
        {
            string Pass = "thiencuong123";
            SendPassword(0xA1, (byte)Pass.Length, Encoding.ASCII.GetBytes(Pass));
        }

        private void button5_Click(object sender, EventArgs e)
        {
            string Pass = "thiencuong123";
            SendPassword(0x06, (byte)Pass.Length, Encoding.ASCII.GetBytes(Pass));
        }

        private void button6_Click(object sender, EventArgs e)
        {
            string Pass = "thiencuong123";
            SendPassword(0x07, (byte)Pass.Length, Encoding.ASCII.GetBytes(Pass));
        }

        private void button7_Click(object sender, EventArgs e)
        {
            string Pass = "CPU";
            SendPassword(0x08, (byte)Pass.Length, Encoding.ASCII.GetBytes(Pass));
        }

        private void button8_Click(object sender, EventArgs e)
        {
            string Pass = "MY PC NAME";
            SendPassword(0x09, (byte)Pass.Length, Encoding.ASCII.GetBytes(Pass));
        }

        private void button9_Click(object sender, EventArgs e)
        {
            SendUsingDate(0x0A, DateTime.Now);
        }
    }
}
