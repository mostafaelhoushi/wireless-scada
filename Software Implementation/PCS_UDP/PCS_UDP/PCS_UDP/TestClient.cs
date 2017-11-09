using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Tiny;
using Tiny.UDP;
using System.Net.Sockets;

namespace PCS_UDP
{
    public partial class TestClient : Form
    {
        public TestClient()
        {
            InitializeComponent();
        }
        TinyClient client;

        private void TestClient_Load(object sender, EventArgs e)
        {
            client = new TinyClient();
            client.ClientPort = 7000;
            client.Encode = EncodingType.ASCII;
            client.Protocol = ProtocolType.Udp; 
        }

        private void btnStart_Click(object sender, EventArgs e)
        {
            client.Start();
            btnStop.Enabled = true;
            btnStart.Enabled = false;
            client.AfterReceive += new System.EventHandler(this.client_AfterReceive);
        }

        private void btnStop_Click(object sender, EventArgs e)
        {
            client.Stop();
            btnStop.Enabled = false;
            btnStart.Enabled = true; 

        }
        private delegate void _DisplayMessage(); 

        private void DisplayMessage() 
       {
           txtMessage.Text += client.Message.ToString();
            //+ ControlChars.CrLf; 
       }

       
        private void client_AfterReceive(object sender, EventArgs e)
        {
            if (this.InvokeRequired)
            {
                this.Invoke(new _DisplayMessage(DisplayMessage));
            }
            else
            {
                DisplayMessage();
            }
        }
    }
}