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
using System.Net;

namespace PCS_UDP
{
    public partial class TestServer : Form
    {
        public TestServer()
        {
            InitializeComponent();
        }

        private void btnSend_Click(object sender, EventArgs e)
        {
            TinyServer server = new TinyServer();
            server.Protocol = ProtocolType.Udp;
            server.ClientAddress = IPAddress.Parse(txtDestIP.Text);
            server.ClientPort = int .Parse(txtDestPort.Text);
            server.Encode = EncodingType.ASCII;
            server.SendMessage(txtMessage.Text);
        }
    }
}