using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace wSound
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            WSounds ws = new WSounds();
            ws.Play("\\Windows \\ My Music\\pegconn.wav", ws.SND_NOSTOP);
        }
    }
    public class WSounds
    {
        [DllImport("Coredll")]
        public static extern bool PlaySound(string fname, int Mod, int flag);

        // these are the SoundFlags we are using here, check mmsystem.h for more
      public int  SND_SYNC = 0x0000; /* play synchronously (default) */
       public int SND_ASYNC = 0x0001; /* play asynchronously */
      public int SND_NODEFAULT = 0x0002; /* silence (!default) if sound not found */
      public int SND_MEMORY = 0x0004; /* pszSound points to a memory file */
       public int SND_LOOP = 0x0008; /* loop the sound until next and PlaySound */
      public int SND_NOSTOP = 0x0010; /* don't stop any currently playing sound */
    public int SND_NOWAIT = 0x00002000; /* don't wait if the driver is busy */
     public int SND_ALIAS = 0x00010000; /* name is a registry alias */
    public int SND_ALIAS_ID = 0x00110000; /* alias is a predefined ID */
  public int SND_FILENAME = 0x00020000; /* name is file name */
     public int SND_RESOURCE = 0x00040004; /* name is resource name or atom */

        public void Play(string fname, int SoundFlags)
        {
            PlaySound(fname, 0, SoundFlags);
        }

        public void StopPlay()
        {
           // PlaySound(null, 0, SND_PURGE);
        }

    }
}