import uPlot from "./node_modules/uplot/dist/uPlot.esm.js"
import EditableValue from "./Utils/EditableValue.js"

let deviceViews = {};
let webSocket = new WebSocket(`ws://${window.location.host}/interface/`);

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
		this.recordDuration = 1000;
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
}
let recordingState = new RecordingState();

class Graph {
	constructor() {
		this.registerViews = {};
		this.uPlot = null;

		this.rebuildGraph();
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
			title: "Test",
			width: 600,
			height: 600,
			series: [
				{}
			],
			axes: [
				{}
			]
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

			let axis = {
				scale: "",
				values: (u, vals, space) => vals.map(v => +v.toFixed())
			};
			options.axes.push(axis);

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

function notifySelectedRegistersChange() {
	graph.onSelectedRegistersChange();	
}

class SelectorColor {

	constructor(index) {
		const goldenRatioInverse = 0.618033988749895;
		this.hue = (goldenRatioInverse * index) % 1.0;
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

			this.selectorLink.click(() => {
				this.selected = !this.selected;
				this.updateSelected();
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

			this.liveValue = $(`<span></span>`);
			this.valueCell.append(this.liveValue);

			this.editValue = $(`<input type="search" value="" class="form-control is-valid" id="inputValid" />`);
			this.valueCell.append(this.editValue);
			this.editValue.hide();

			this.editValueError = $(`<div class="invalid-feedback">Sorry, that username's taken. Try another?</div>`);
			this.valueCell.append(this.editValueError);
			this.editValueError.hide();

			this.liveValue.click(() => {
				this.openEditor();
			});

			this.editValue.focusout(() => {
				this.closeEditor();
			});

			this.editValue.on('search', () => {
				if(this.validate()) {
					this.deviceView.pushRegisterValue(this.registerIndex, eval(this.editValue.val()));
					this.closeEditor();
				}
			});

			this.editValue.on('keydown', (args) => {
				if(args.key == 'Escape') {
					this.closeEditor();
				}
			});

			this.editValue.on('input', () => {
				this.validate();
			});
		}

		this.actionsCell = $(`<td></td>`);
		this.tableRow.append(this.actionsCell);
	}

	openEditor() {
		this.editValue.show();
		this.editValueError.hide();
		this.liveValue.hide();
		this.editValue.val(this.liveValue.text());
		this.editValue.focus();
		this.editValue.select();
	}

	closeEditor() {
		this.editValue.hide();
		this.editValueError.hide();
		this.liveValue.show();
	}

	setInfo(registerInfo) {
		this.registerInfo = registerInfo;
		if('name' in registerInfo) {
			this.nameCell.text(registerInfo['name']);
		}
	}

	setValue(value) {
		this.liveValue.text(value);
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

	validate() {
		try {
			let result = eval(this.editValue.val());
			if(isNaN(result)) {
				throw("Result is not a number");
			}
			if(this.registerInfo) {
				if(this.registerInfo.range) {
					if(result < this.registerInfo.range.min) {
						throw(`Value (${result}) is less than allowed minimum (${this.registerInfo.range.min})`);
					}
					if(result > this.registerInfo.range.max) {
						throw(`Value (${result}) is greater than allowed maximum (${this.registerInfo.range.max})`);
					}
				}
			}
			this.editValueError.hide();
			this.editValue.addClass('is-valid');
			this.editValue.removeClass('is-invalid');
			return true;
		}
		catch(exception) {
			this.editValueError.show();
			this.editValueError.text(exception);
			this.editValue.removeClass('is-valid');
			this.editValue.addClass('is-invalid');
			return false;
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
};

class DeviceView {
	constructor(hardwareID) {
		this.registerViews = {}
		this.hardwareID = hardwareID;

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
	}

	dispose() {
		deviceViewArea.remove(this.view);
	}

	getOrMakeRegister(registerIndex) {
		if(!(registerIndex in this.registerViews)) {
			let registerView = new RegisterView(this.table, registerIndex, this);
			this.registerViews[registerIndex] = registerView;
			return registerView;
		}
		else {
			return this.registerViews[registerIndex];
		}
	}

	onMessage(message) {
		if('register_info' in message) {
			for(let registerIndex in message['register_info']) {
				let registerView = this.getOrMakeRegister(registerIndex);
				registerView.setInfo(message['register_info'][registerIndex]);
			}
		}
		if('register_values' in message) {
			// get a timestamp if there is one
			let upTime = null;
			if('42' in message['register_values']) {
				upTime = message['register_values']['42'];
			}

			// push the data into the RegisterView objects
			for(let registerIndex in message['register_values']) {
				let registerView = this.getOrMakeRegister(registerIndex);
				let value = message['register_values'][registerIndex];

				registerView.setValue(value);

				if(upTime) {
					registerView.recordSample(upTime, value)
				}
			}

			// notify graph to update
			graph.updateData();
		}
	}

	pushRegisterValue(registerIndex, value) {
		let registerValues = {};
		registerValues[registerIndex] = value;

		let request = [
			{
				"hardware_id" : this.hardwareID, 
				"content" : {
					"register_values" : registerValues
				}
			}
		];

		webSocket.send(JSON.stringify(request));
	}
};

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