using MuscleMemory;
using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using VVVV.Core.Logging;
using VVVV.PluginInterfaces.V2;

namespace VVVV.MuscleMemory
{
	#region PluginInfo
	[PluginInfo(Name = "MotionControl",
				Category = "MuscleMemory",
				Help = "Move motors with max velocity, acceleration",
				AutoEvaluate = true
		)]
	#endregion PluginInfo
	public class MotionControlNode : IPluginEvaluate, IDisposable
	{
		public class MotorController
		{
			bool FNeedsRequestPosition = true;
			bool FNeedsTakeIncomingPosition = false;
			bool FHasValidTracking = false;

			Motor FMotor = null;
			double FMaxAcceleration = 1.0;
			double FMaxVelocity = 1.0;

			double FCurrentPosition = 0.0;
			double FCurrentVelocity = 0.0;

			double FLatestActualPosition = 0.0;

			double FTargetPosition = 0.0;

			DateTime FLastTimeTick = DateTime.Now;

			public void SetConfiguration(Motor motor, double maxVelocity, double maxAcceleration)
			{
				if (motor != this.FMotor)
				{
					this.Clear();
					this.FMotor = motor;
					if (motor != null)
					{
						motor.OnRegisterReceive += this.MotorRegisterReceive;
					}
				}

				this.FMaxVelocity = maxVelocity;
				this.FMaxAcceleration = maxAcceleration;
			}

			public void Clear()
			{
				if (this.FMotor != null)
				{
					this.FMotor.OnRegisterReceive -= this.MotorRegisterReceive;
					this.FMotor = null;
					this.FNeedsRequestPosition = true;
					this.FNeedsTakeIncomingPosition = false;
					this.FHasValidTracking = false;
				}
			}

			public void MotorRegisterReceive(Messages.RegisterType registerType, int value)
			{
				if (registerType == Messages.RegisterType.MultiTurnPosition)
				{
					this.FNeedsTakeIncomingPosition = true;
				}
			}

			public void SetTarget(double position)
			{
				this.FTargetPosition = position;
			}

			public double CurrentIntendedPosition
			{
				get
				{
					return this.FCurrentPosition;
				}
			}

			public double CurrentIntendedVelocity
			{
				get
				{
					return this.FCurrentVelocity;
				}
			}

			public double LatestActualPosition
			{
				get
				{
					return this.FLatestActualPosition;
				}
			}

			public void RequestPosition()
			{
				this.FNeedsRequestPosition = true;
			}

			public void Update()
			{
				// Request positions from actual motor
				if (this.FNeedsRequestPosition)
				{
					this.FMotor.RequestRegister(Messages.RegisterType.MultiTurnPosition, false);
					this.FNeedsRequestPosition = false;
				}

				// Take incoming positions from actual motor
				if (this.FNeedsTakeIncomingPosition)
				{
					var position = (double)this.FMotor.CachedRegisterValues[Messages.RegisterType.MultiTurnPosition] / (double)(1 << 14);
					this.FLatestActualPosition = position;
					this.FCurrentPosition = position;
					this.FCurrentVelocity = 0.0;
					this.FNeedsTakeIncomingPosition = false;
				}

				// Don't continue if we don't have tracking
				if (!this.FHasValidTracking)
				{
					return;
				}

				// Get dt
				var now = DateTime.Now;
				var timeSinceLastTick = now - this.FLastTimeTick;
				var dt = timeSinceLastTick.TotalSeconds;
				this.FLastTimeTick = DateTime.Now;

				// Get current position
				this.FCurrentPosition += this.FCurrentVelocity * dt;
				var positionDelta = this.FTargetPosition - this.FCurrentPosition;

				// Calc how much we can change our velocity in a time step
				var velocityChangePerTimeStep = this.FMaxAcceleration * dt;

				// Check if need to deccelerate
				var timeToDecellerateFully = Math.Abs(this.FCurrentVelocity / this.FMaxAcceleration);
				var positionAtEndOfDecceleration = this.FCurrentPosition + (this.FCurrentVelocity * timeToDecellerateFully / 2);
				bool needsDecellerate = false;
				if (positionDelta > 0 && this.FCurrentVelocity > 0 && positionAtEndOfDecceleration > this.FTargetPosition)
				{
					needsDecellerate = true;
				}
				else if (positionDelta < 0 && this.FCurrentVelocity < 0 && positionAtEndOfDecceleration < this.FTargetPosition)
				{
					needsDecellerate = true;
				}
				if (needsDecellerate)
				{
					if (this.FCurrentVelocity > velocityChangePerTimeStep)
					{
						this.FCurrentVelocity -= velocityChangePerTimeStep;
					}
					else if (this.FCurrentVelocity < -velocityChangePerTimeStep)
					{
						this.FCurrentVelocity += velocityChangePerTimeStep;
					}
					else
					{
						this.FCurrentVelocity = 0;
					}
				}

				// Check if needs to accelerate
				if (!needsDecellerate)
				{
					bool needsAccelerate = false;
					if (positionDelta > 0 && this.FCurrentPosition + this.FCurrentVelocity * dt < this.FTargetPosition)
					{
						needsAccelerate = true;
					}
					else if (positionDelta < 0 && this.FCurrentPosition + this.FCurrentVelocity * dt > this.FTargetPosition)
					{
						needsAccelerate = true;
					}
					if (needsAccelerate)
					{
						if (positionDelta > 0)
						{
							this.FCurrentVelocity += velocityChangePerTimeStep;
						}
						else
						{
							this.FCurrentVelocity -= velocityChangePerTimeStep;
						}
					}
				}

				// Clamp velocity to max
				if (this.FCurrentVelocity > this.FMaxVelocity)
				{
					this.FCurrentVelocity = this.FMaxVelocity;
				}
				else if (this.FCurrentVelocity < -this.FMaxVelocity)
				{
					this.FCurrentVelocity = -this.FMaxVelocity;
				}

				// Send variables to the motor
				this.FMotor.SetRegister(Messages.RegisterType.TargetPosition, (int)((double)(1 << 14) * this.FCurrentPosition), false);
				this.FMotor.SetRegister(Messages.RegisterType.TargetVelocity, (int)((double)(1 << 14) * this.FCurrentVelocity), false);
			}
		}

		#region fields & pins
		[Input("Bus Group", IsSingle = true)]
		public ISpread<BusGroup> FInBusGroup;

		[Input("ID", DefaultValue = 1)]
		public ISpread<int> FInID;

		[Input("Max Velocity", DefaultValue = 1.0)]
		public ISpread<double> FInMaxVelocity;

		[Input("Max Acceleration", DefaultValue = 1.0)]
		public ISpread<double> FInMaxAcceleration;

		[Input("Update Rate", DefaultValue = 100)]
		public ISpread<double> FInUpdateRate;

		[Input("Position")]
		public ISpread<double> FInPosition;

		[Input("Go", IsBang = true)]
		public ISpread<bool> FInGo;

		[Input("Force Get Position", IsBang = true)]
		public ISpread<bool> FInForceGetPosition;

		[Output("Bus Group")]
		public ISpread<BusGroup> FOutBusGroup;

		[Output("Position")]
		public ISpread<double> FOutPosition;

		[Output("Velocity")]
		public ISpread<double> FOutVelocity;

		[Output("Last Seen Position")]
		public ISpread<double> FOutLastSeenPosition;

		[Import()]
		public ILogger FLogger;

		public object FLockMotorControllers = new object();
		public List<MotorController> FMotorControllers = new List<MotorController>();

		public Thread FThread;
		public bool FThreadClosing = false;
		public double FThreadInterval = 0.01;
		#endregion fields & pins

		MotionControlNode()
		{
			this.FThread = new Thread(this.ThreadedFunction);
			this.FThread.Start();
		}

		//called when data for any output pin is requested
		public void Evaluate(int SpreadMax)
		{
			FOutBusGroup[0] = FInBusGroup[0];

			var busGroup = FInBusGroup[0];
			if (busGroup is BusGroup)
			{
				lock(this.FLockMotorControllers)
				{
					// Deinit and clear out motor controllers above the indexes we have
					if (this.FMotorControllers.Count > FInID.SliceCount)
					{
						for (int i = FMotorControllers.Count - 1; i >= this.FInID.SliceCount; i--)
						{
							this.FMotorControllers[i].Clear();
							this.FMotorControllers[i] = null;
							this.FMotorControllers.RemoveAt(i);
						}
					}

					// Init and add motor controllers
					if (FInID.SliceCount < this.FMotorControllers.Count)
					{
						for (int i = this.FMotorControllers.Count; i < this.FInID.SliceCount; i++)
						{
							var motorController = new MotorController();
							this.FMotorControllers.Add(motorController);
						}
					}

					// Set config on all motor controllers
					for (int i = 0; i < FInID.SliceCount; i++)
					{
						var motorController = this.FMotorControllers[i];
						var motor = busGroup.FindMotor(this.FInID[i]);
						motorController.SetConfiguration(motor, this.FInMaxVelocity[i], this.FInMaxAcceleration[i]);

						if (FInForceGetPosition[i])
						{
							motorController.RequestPosition();
						}
					}

					// Output report info
					this.FOutPosition.SliceCount = this.FMotorControllers.Count;
					this.FOutVelocity.SliceCount = this.FMotorControllers.Count;
					this.FOutLastSeenPosition.SliceCount = this.FMotorControllers.Count;
					for(int i=0; i< this.FMotorControllers.Count; i++)
					{
						var motorController = this.FMotorControllers[i];
						this.FOutPosition[i] = motorController.CurrentIntendedPosition;
						this.FOutVelocity[i] = motorController.CurrentIntendedVelocity;
						this.FOutLastSeenPosition[i] = motorController.LatestActualPosition;
					}
				}
			}

			this.FThreadInterval = 1.0 / FInUpdateRate[0];
		}

		public void ThreadedFunction()
		{
			while(!this.FThreadClosing)
			{
				lock(this.FLockMotorControllers)
				{
					foreach(var motorController in this.FMotorControllers)
					{
						try
						{
							motorController.Update();
						}
						catch(Exception e)
						{
							Debug.WriteLine(e.Message);
						}
					}
				}
				Thread.Sleep(TimeSpan.FromSeconds(this.FThreadInterval));
			}
		}

		public void Dispose()
		{
			this.FThreadClosing = true;
			this.FThread.Join();
		}
	}
}
