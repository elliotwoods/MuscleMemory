import uPlot from "../node_modules/uplot/dist/uPlot.esm.js"

class EncoderCalibration {
	constructor() {
		this.uPlot = null;
		this.device = null;
	}

	setDevice(device) {
		this.device = device;
		this.device.sendRequest({
			"request_encoder_calibration_data" : null
		});
	}

	update(data) {
		this.data = data;
		if(this.uPlot == null) {
			this.buildGraph();
		}
		else {
			this.updateGraph();
		}

		$("#encoderCalibrationInfo").empty();
		{
			let infoList = $(`<ul></ul>`);
			$("#encoderCalibrationInfo").append(infoList);

			for(let key in data) {
				if(key != 'encoderValuePerStepCycle') {
					infoList.append(`<li><b>${key} : </b>${data[key]}`);
				}
			}
		}
		
	}

	buildGraph() {
		let options = {
			width: 600,
			height: 600,
			scale : {
				x : {
					time: false
				}
			},
			series: [
				{
					label : "Step index",
					value : (u, v) => v == null ? "-" : v.toFixed()
				},
				{
					label : "Encoder value",
					value : (u, v) => v == null ? "-" : v.toFixed()
				}
			],
			axes : [
				{
					values : (u, vals, space) => vals.map(x => x.toFixed())
				},
				{
					values : (u, vals, space) => vals.map(x => x.toFixed())
				}
			],
			cursor : {
				drag : {
					x : true,
					y : true,
					uni : 50
				}
			}
		};

		this.uPlot = new uPlot(options, this.getDataSeries(), document.getElementById("encoderCalibration"));
	}

	updateGraph() {
		this.uPlot.setData(this.getDataSeries());
	}

	getDataSeries() {
		let data;
		if(this.data) {
			data = [
				[],
				[]
			];

			for(let i=0; i<this.data.encoderValuePerStepCycle.length; i++) {
				data[0].push(i);
				data[1].push(this.data.encoderValuePerStepCycle[i]);
			}
		}
		else {
			data = [
				[0],
				[0]
			];
		}
		return data;
	}
};

export default EncoderCalibration;