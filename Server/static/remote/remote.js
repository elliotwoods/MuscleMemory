const motorCount = 80;

class Motor {
	constructor(parent, index) {
		this.selected = false;

		this.li = $(`<li class="list-group-item list-group-item-action"/>`);
		parent.append(this.li);
		this.li.text(index);

		this.li.click(() => {
			this.selected ^= true;
			this.refresh();
		});
	}

	refresh() {
		if (this.selected) {
			this.li.addClass("active");
		}
		else {
			this.li.removeClass("active");
		}
	}
}

let motors = {};
let deviceMotionInitialised = false;

function initialiseDeviceMotion() {
	if (typeof DeviceMotionEvent.requestPermission === 'function') {
		DeviceMotionEvent.requestPermission()
			.then(response => {
				if (response == 'granted') {
					window.addEventListener('devicemotion', onDeviceOrientation);
				}
			}).catch(console.error);
	}
	else
	{
		window.addEventListener('devicemotion', onDeviceOrientation);
	}
	deviceMotionInitialised = true;
}

class MoveButton {
	constructor(element) {
		this.isMoving = true;

		this.touchesPrior = {};

		this.element = element;
		this.element.on('touchstart mousedown', (e) => {
			// if(!deviceMotionInitialised) {
			// 	initialiseDeviceMotion();
			// }
			this.isMoving = true;
			this.refresh();
			e.preventDefault();

			for(var touch of  e.changedTouches) {
				this.touchesPrior[touch.identifier] = touch;
				this.element.text("NO");
			}
		});
		this.element.on('touchend click', (e) => {
			this.isMoving = false;
			this.refresh();
		});

		this.element.on('touchmove', (e) => {
			let movement = 0;

			for(let touch of e.changedTouches) {
				if(touch.identifier in this.touchesPrior) {
					movement += touch.pageY - this.touchesPrior[touch.identifier].pageY;
					this.touchesPrior[touch.identifier] = touch;
				}
			}

			// get selection
			let selection = [];
			for(let ID in motors) {
				if(motors[ID].selected) {
					selection.push(ID);
				}
			}
			if(selection.length > 0) {
				let selectionString = selection.join(",");

				$.get(`/remote/move/${selectionString}/${movement}`);
				this.element.text(movement);
			}
			
		});
	}

	refresh() {
		if (this.isMoving) {
			this.element.text("Move");
			this.element.removeClass("btn-secondary");
			this.element.addClass("btn-primary");
		}
		else {
			this.element.text("Move");
			this.element.removeClass("btn-primary");
			this.element.addClass("btn-secondary");
		}
	}
}
let moveButton;

function onDeviceOrientation(args) {
	moveButton = $("#moveButton");
	moveButton.text(JSON.stringify(args.rotationRate));
}

$(document).ready(() => {
	for (let i = 1; i <= motorCount; i++) {
		let container;
		if(i <= 40) {
			container = $("#motorListLeft");
		}
		else {
			container = $("#motorListRight");
		}
		motors[i] = new Motor(container, i);
	};

	moveButton = new MoveButton($("#moveButton"));
});