#region usings
using System;
using System.IO;
using System.ComponentModel.Composition;

using VVVV.PluginInterfaces.V1;
using VVVV.PluginInterfaces.V2;
using VVVV.Utils.VColor;
using VVVV.Utils.VMath;

using VVVV.Core.Logging;
#endregion usings

namespace VVVV.Nodes {
	public class CanMessage
	{
		public UInt32 Flags;
		public UInt32 Identifier;
		public Byte Length;
		public Byte[] Data = new byte[8];
	}
}