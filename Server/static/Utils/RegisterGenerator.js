import showModal from "./Modal.js"

let registerGenerators = [];
let timeStart = new Date();
let time = 0;

class RegisterGenerator {
	constructor(parent, register) {
		registerGenerators.push(this);
		this.parent = parent;
		this.register = register;
		this.enabled = false;

		this.button = $(`<button id="clearButton" class="btn btn-secondary" title="Animate" />`);
		parent.append(this.button);

		this.icon = $(`<i class="fas fa-running" />`);
		this.button.append(this.icon);

		this.button.click(() => {
			this.openModal();
		});

		this.modalBody = $(`<div></div>`);
		{
			{
				let group = $(`<div class="custom-control custom-switch"></div>`);
				this.modalBody.append(group);

				this.toggleEnabled = $(`<input type="checkbox" class="custom-control-input" id="animationEnabled">`);
				group.append(this.toggleEnabled);
				group.append(`<label class="custom-control-label" for="animationEnabled">Enabled</label>`);

				this.toggleEnabled.change(() => {
					this.enabled = this.toggleEnabled.is(":checked");

					if(this.enabled) {
						this.button.removeClass("btn-secondary");
						this.button.addClass("btn-primary");
					}
					else {
						this.button.removeClass("btn-primary");
						this.button.addClass("btn-secondary");
					}
				});
			}
			
			this.modalBody.append(`<span class="codeTip">t = time in seconds</span>`);

			{
				let group = $(`<div class="form-group"></div>`);
				this.modalBody.append(group);

				group.append(`<label for="animationCode">Code</label>`);

				this.codeTextArea = $(`<textarea spellcheck="false" class="form-control codeEditor" id="animationCode" rows="3">0</textarea>`);
				this.codeTextArea.change(() => {
					this.validate();
				})
				group.append(this.codeTextArea);

				this.codeTextAreaError = $(`<div class="invalid-feedback"></div>`);
				group.append(this.codeTextAreaError);
			}

			this.resultDisplay = $(`<span class="resultDisplay"></span>`);
			this.modalBody.append(this.resultDisplay);
		}
	}

	update() {
		if(this.enabled) {
			if(this.validate()) {
				this.register.pushValue(this.getValue());
			}
		}
		
	}
	
	show() {
		this.button.show();
	}

	hide() {
		this.button.hide();
	}

	openModal() {
		showModal(`Register generator : ${this.register.registerInfo.name}`).append(this.modalBody);
	}

	getValue() {
		let result = parseInt(this.evaluate(this.codeTextArea.val()));
		if(isNaN(result)) {
			throw("Does not evaluate to a number");
		}
		return result;
	}

	validate() {
		try {
			let result = this.getValue();

			this.codeTextArea.addClass('is-valid');
			this.codeTextArea.removeClass('is-invalid');
			this.codeTextAreaError.hide();
			this.resultDisplay.show();
			this.resultDisplay.text(result);
			return true;
		}
		catch (error) {
			this.codeTextArea.addClass('is-invalid');
			this.codeTextArea.removeClass('is-valid');
			this.codeTextAreaError.show();
			this.codeTextAreaError.text(error);
			this.resultDisplay.hide();
			return false;
		}
	}

	evaluate(text) {
		return eval(`let t=${time};` + text).toFixed();
	}
}

function updateAllRegisterGenerators() {
	let timeNow = (new Date()) - timeStart;
	time = timeNow / 1000.0; // convert to seconds and store in global area 

	for(let i in registerGenerators) {
		registerGenerators[i].update();
	}
}
setInterval(updateAllRegisterGenerators, 100);

export default RegisterGenerator;