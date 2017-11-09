using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace PCS_UDP
{
    public partial class Start : Form
    {
        public Start()
        {
            InitializeComponent();
        }

        private void Start_Load(object sender, EventArgs e)
        {
            TestServer s = new TestServer();
            TestClient c = new TestClient();
            s.Show();
            c.Show();
            s.Left = 500;
            s.Top = 100;
            c.Left = 50;
            c.Top = 100;

        }
    }
}