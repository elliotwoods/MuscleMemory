import uPlot from "./node_modules/uplot/dist/uPlot.esm.js"
import EditableValue from "./Utils/EditableValue.js"
import RegisterGenerator from "./RegisterActions/RegisterGenerator.js"
import EncoderCalibration from "./Elements/EncoderCalibration.js"

let deviceViews = {};
let webSocket = new WebSocket(`ws://${window.location.host}/interface/`);
let encoderCalibration = null;

function printUs(seconds) {
	let minutes = Math.floor(seconds / 60);
	seconds %= 60;
	let hours = Math.floor(minutes / 60);
	minutes %= 60;
	let days = Math.floor(hours / 24);
	hours %= 24;

	if(days > 0) {
		return `${days} days\n${hours}h ${minutes}m ${seconds.toFixed()}s`;
	}
	else if(hours > 0) {
		return `${hours}h ${minutes}m ${seconds.toFixed()}s`;
	}
	else if(minutes > 0) {
		return `${minutes}m ${seconds.toFixed()}s`;
	}
	else if(seconds > 0) {
		return `${seconds.toFixed(2)}s`;
	}
}

const recordStates = {
	Playing : {
		button : $("#playButton"),
		selectedClass : "btn-primary"
	},
	Recording : {
		button : $("#recordButton"),
		selectedClass : "btn-danger"
	},
	Paused : {
		button : $("#pauseButton"),
		selectedClass : "btn-primary"
	}
}

class RecordingState {
	constructor() {
		this.recordState = recordStates.Playing;
		this.recordDuration = 300;
		this.initInterface();
		this.editRecordDuration = new EditableValue($("#recordDuration"), this.recordDuration, (value) => {
			this.recordDuration = value;
		}, (value) => {
			if(value < 1) {
				throw("Duration must be at least 1 frame");
			}
		});
	}

	initInterface() {
		for(let recordStateName in recordStates) {
			let recordState = recordStates[recordStateName];
			recordState.button.click(() => {
				this.recordState = recordState;
				this.updateInterface();
			});
		}
		$("#clearButton").click(() => {
			this.clearRecordings();
		});
		this.updateInterface();
	}

	updateInterface() {
		for(let recordStateName in recordStates) {
			let recordState = recordStates[recordStateName];
			if(this.recordState == recordState) {
				recordState.button.addClass(recordState.selectedClass);
			}
			else {
				recordState.button.removeClass(recordState.selectedClass);
			}
		}
	}

	clearRecordings() {
		for(let hardwareID in deviceViews) {
			let deviceView = deviceViews[hardwareID];
			for(let registerIndex in deviceView.registerViews) {
				let registerView = deviceView.registerViews[registerIndex];

				registerView.recordSamples = [];
			}
		}
	}
}
let recordingState = new RecordingState();

class Graph {
	constructor() {
		this.registerViews = {};
		this.uPlot = null;
		this.parent = $("#graph");

		this.rebuildGraph();

		this.parent.resize(() => {
			this.uPlot.resize(this.parent.width(), 600);
		});
	}

	onSelectedRegistersChange() {
		let newRegisterViews = {};
		for(let hardwareID in deviceViews) {
			let deviceView = deviceViews[hardwareID];
			for(let registerIndex in deviceView.registerViews) {
				let registerView = deviceView.registerViews[registerIndex];
				if(registerView.selected) {
					newRegisterViews[`/${hardwareID}/${registerIndex}`] = registerView;
				}
			}
		}

		if(newRegisterViews != this.registerViews) {
			 this.registerViews = newRegisterViews;
			 this.rebuildGraph();
		}
	}

	rebuildGraph() {
		if(this.uPlot != null) {
			this.uPlot.destroy();
			this.uPlot = null;
		}

		if(this.registerViews == {}) {
			// Nothing to draw
			return;
		}

		let options = {
			width: this.parent.width(),
			height: 600,
			scales : {
				x : {
					time: false
				}
			},
			series: [
				{
					label : "UpTime",
					value : (u, v) => v == null ? "-" : printUs(v)
				}
			],
			axes: [
				{
					values : (u, vals, space) => vals.map(printUs)
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
		let data = [
			[0]
		];
		for(let registerAddress in this.registerViews) {
			let series = {
				label: this.registerViews[registerAddress].registerInfo.name,
				scale: "",
				value : (u, v) => v == null ? "-" : v.toFixed(),
				stroke : this.registerViews[registerAddress].color.getCSS()
			};
			options.series.push(series);

			let axes = {
				scale: "",
				values: (u, vals, space) => vals.map(v => +v.toFixed())
			}
			options.axes.push(axes);

			data.push([0]);
		}
		this.uPlot = new uPlot(options, data, document.getElementById("graph"));
		this.updateData();
	}

	updateData() {
		if(this.uPlot) {
			// Gather data
			let registerAddresses = Object.keys(this.registerViews);

			let timestamps = [];
			let gatheredSamples = new Array(registerAddresses.length);
			for(let i=0; i<gatheredSamples.length; i++) {
				gatheredSamples[i] = new Array();
			}

			for(let registerAddressIndex in registerAddresses) {
				let registerAddress = registerAddresses[registerAddressIndex];
				let registerSamples = gatheredSamples[registerAddressIndex];

				for(let recordSample of this.registerViews[registerAddress].recordSamples) {
					// If we haven't yet gathered any recordings at this timestamp
					let timestampIndex = timestamps.indexOf(recordSample.time);
					if(timestampIndex == -1) {
						// Then add to the timestamps
						timestamps.push(recordSample.time);
						timestampIndex = timestamps.length - 1;
						
						// And put a null placeholder onto each data array
						gatheredSamples.map((sampleSet) => sampleSet.push(null));
					}

					registerSamples[timestampIndex] = recordSample.value;
				}
			}

			let data = [
				timestamps
				, ...gatheredSamples
			];

			this.uPlot.setData(data);
		}
	}
}

let graph = new Graph();
window.graph = graph;

function notifySelectedRegistersChange() {
	graph.onSelectedRegistersChange();	
}

class SelectorColor {

	constructor(index) {
		const goldenRatioInverse = 0.618033988749895;
		this.hue = (goldenRatioInverse * (index + 1)) % 1.0;
		this.saturation = '90%';
		this.luminance = '50%';
	}

	getCSS() {
		return `hsl(${this.hue * 360.0}, ${this.saturation}, ${this.luminance})`;
	}
}

// from https://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically/
// https://www.w3schools.com/colors/colors_hsl.asp
function indexToColor(index) {
	
}

class RegisterView {
	constructor(parentTable, registerIndex, deviceView) {
		this.registerIndex = registerIndex;
		this.registerInfo = null;
		this.selected = false;
		this.editable = false;
		this.recordSamples = [];

		this.deviceView = deviceView;

		this.tableRow = $(`<tr></tr>`);
		parentTable.append(this.tableRow);

		// Selector
		{
			this.color = new SelectorColor(registerIndex);

			this.selectCell = $(`<td></td>`); 
			this.tableRow.append(this.selectCell);

			this.selectorLink = $(`<a href="#"></a>`);
			this.selectCell.append(this.selectorLink);

			this.selectorLink.click((event) => {
				this.selected = !this.selected;
				this.updateSelected();
				event.preventDefault();
			});

			this.selector = $(`<div class="selector"></div>`);
			this.selectorLink.append(this.selector);
			this.selector.css("border-color", this.color.getCSS());

			this.updateSelected();
		}

		this.nameCell = $(`<th scope="row"></th>`);
		this.tableRow.append(this.nameCell);

		this.tableRow.append(`<td>${registerIndex}</td>`);

		{
			this.valueCell = $(`<td></td>`);
			this.tableRow.append(this.valueCell);

			this.editableValue = new EditableValue(this.valueCell, 0,
				// Set value
				(value) => {
					this.pushValue(value);
				},

				// Validate
				(value) => {
					if(this.registerInfo) {
						if(this.registerInfo.range) {
							if(value < this.registerInfo.range.min) {
								throw(`Value (${result}) is less than allowed minimum (${this.registerInfo.range.min})`);
							}
							if(value > this.registerInfo.range.max) {
								throw(`Value (${result}) is greater than allowed maximum (${this.registerInfo.range.max})`);
							}
						}
					}
				});

			this.defaultView = $(`<span class="defaultValue" />`);
			this.valueCell.append(this.defaultView);
			this.defaultView.hide();
		}

		this.actionsCell = $(`<td></td>`);
		this.tableRow.append(this.actionsCell);

		this.storeDefaultButton = $(`<button class="btn btn-secondary btn-sm" title="Store default"><i class="fas fa-save" /></button>`);
		this.actionsCell.append(this.storeDefaultButton);
		this.storeDefaultButton.click(() => {
			this.storeDefault();
		});

		this.hideButton = $('<button class="btn btn-secondary btn-sm" title="Hide"/>');
		this.actionsCell.append(this.hideButton);
		this.hideButton.append(`<i class="fas fa-eye-slash"></i>`);
		this.hideButton.click(() => {
			this.tableRow.hide();
		});

		this.registerGenerator = new RegisterGenerator(this.actionsCell, this);
	}

	pushValue(value) {
		this.deviceView.pushRegisterValue(this.registerIndex, value);
	}

	setInfo(registerInfo) {
		this.registerInfo = registerInfo;
		if('name' in registerInfo) {
			this.nameCell.text(registerInfo['name']);
		}
		if('access' in registerInfo) {
			this.editable = registerInfo['access'] == 1;
			this.editableValue.setEditEnabled(this.editable);
		}
		if('defaultValue' in registerInfo) {
			this.defaultView.text(this.registerInfo['defaultValue']);
			if(this.editable) {
				this.defaultView.show();
			}
		}

		this.refreshActions();
	}

	setValue(value) {
		this.editableValue.setValue(value);
	}

	storeDefault() {
		this.deviceView.sendRequest({
			'register_save_default' : [this.registerIndex]
		});
	}

	recordSample(time, value) {
		this.recordSamples.push({
			time : time,
			value : value
		});
		if(this.recordSamples.length > recordingState.recordDuration) {
			this.recordSamples = this.recordSamples.slice(this.recordSamples.length - recordingState.recordDuration);
		}
	}

	updateSelected() {
		if(this.selected) {
			this.selector.css("background", this.color.getCSS());
		}
		else {
			this.selector.css("background", "");
		}
		notifySelectedRegistersChange();
	}

	refreshActions() {
		if(this.editable) {
			this.registerGenerator.show();
			this.storeDefaultButton.show();
		}
		else {
			this.registerGenerator.hide();
			this.storeDefaultButton.hide();
		}
	}
};

class DeviceView {
	constructor(hardwareID) {
		this.registerViews = {}
		this.hardwareID = hardwareID;
		this.encoderCalibration = null;

		this.view = $(`
			<div class="device">
				<h2>${hardwareID}</h2>
			</div>`);
		$("#clients").append(this.view);

		this.table = $(`
			<table class="table table-hover">
				<thead>
					<th scope="col"></th>
					<th scope="col">Name</th>
					<th scope="col">Index</th>
					<th scope="col">Value</th>
					<th scope="col">Actions</th>
				</thead>
			</table>`);
		this.view.append(this.table);

		encoderCalibration.setDevice(this);
	}

	dispose() {
		deviceViewArea.remove(this.view);
	}

	getOrMakeRegister(registerIndex) {
		let key = String(registerIndex);
		if(!(key in this.registerViews)) {
			let registerView = new RegisterView(this.table, registerIndex, this);
			this.registerViews[key] = registerView;
			return registerView;
		}
		else {
			return this.registerViews[registerIndex];
		}
	}

	onMessage(message) {
		if('register_info' in message) {
			for(let key in message['register_info']) {
				let registerIndex = parseInt(key);
				let registerView = this.getOrMakeRegister(registerIndex);
				registerView.setInfo(message['register_info'][key]);
			}
		}
		if('register_values' in message) {
			// get a timestamp if there is one
			let upTime = null;
			let graphNeedsUpdate = false;
			if('42' in message['register_values']) {
				upTime = message['register_values']['42'] / 1000;
			}

			// push the data into the RegisterView objects
			for(let key in message['register_values']) {
				let registerIndex = parseInt(key);
				let registerView = this.getOrMakeRegister(registerIndex);
				let value = message['register_values'][key];

				registerView.setValue(value);

				if(upTime && recordingState.recordState != recordStates.Paused) {
					registerView.recordSample(upTime, value)
					graphNeedsUpdate = true;
				}
			}

			// notify graph to update
			if(graphNeedsUpdate) {
				graph.updateData();
			}
		}
		if('encoder_calibration' in message) {
			this.encoderCalibration = message['encoder_calibration'];
			if(encoderCalibration.device == this) {
				encoderCalibration.update(message['encoder_calibration']);
			}
		}
	}

	pushRegisterValue(registerIndex, value) {
		let registerValues = {};
		registerValues[registerIndex] = value;

		this.sendRequest({
			"register_values" : registerValues
		});
	}

	sendRequest(request) {
		let fullRequest = [
			{
				"hardware_id" : this.hardwareID, 
				"content" : request
			}
		];

		webSocket.send(JSON.stringify(fullRequest));
	}
};

$(document).ready(() => {
	encoderCalibration = new EncoderCalibration();

	webSocket.onmessage = (args) => {
		let dataDeviceArray = JSON.parse(args.data);
		for(let dataDevice of dataDeviceArray) {
			if('hardware_id' in dataDevice) {
				let hardwareID = dataDevice['hardware_id'];
				if(!(hardwareID in deviceViews)) {
					deviceViews[hardwareID] = new DeviceView(hardwareID);
					console.log(`Adding HW ID : ${hardwareID}`);
				}
				deviceViews[hardwareID].onMessage(dataDevice.content);
			}
		}
	};
});
