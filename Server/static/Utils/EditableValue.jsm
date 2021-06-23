class EditableValue {
	constructor(parent, defaultValue, onSetValue, validateFunction) {
		this.parent = parent;

		this.valueDisplay = $(`<a href="#" class="editableValue"></a>`);
		this.parent.append(this.valueDisplay);
		this.valueDisplay.click(() => {
			this.openEditor();
			return false;
		});

		this.valueDisplayNonInteractive = $(`<span class="liveValue"></span>`);
		this.parent.append(this.valueDisplayNonInteractive);
		this.valueDisplayNonInteractive.hide();
		
		this.editValue = $(`<input type="search" value="" class="form-control is-valid" id="inputValid" />`);
		this.parent.append(this.editValue);
		this.editValue.hide();
		this.editValue.focusout(() => {
			this.closeEditor();
		});
		this.editValue.on('search', () => {
			if(this.validate()) {
				this.pushValue();
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

		this.extraFeedback = $(`<div class="valid-feedback"></div>`);
		this.parent.append(this.extraFeedback);
		this.extraFeedback.hide();

		this.editValueError = $(`<div class="invalid-feedback"></div>`);
		this.parent.append(this.editValueError);
		this.editValueError.hide();

		this.setValue(defaultValue);

		this.customValidateFunction = validateFunction;
		this.onSetValue = onSetValue;
	}

	openEditor() {
		this.editValue.show();
		this.extraFeedback.hide();
		this.editValueError.hide();
		this.valueDisplay.hide();
		this.editValue.val(this.valueDisplay.text());
		this.editValue.focus();
		this.editValue.select();

		this.validate();
	}

	closeEditor() {
		this.editValue.hide();
		this.extraFeedback.hide();
		this.editValueError.hide();
		this.valueDisplay.show();
	}

	setEditEnabled(editEnabled) {
		if(editEnabled) {
			this.valueDisplay.show();
			this.valueDisplayNonInteractive.hide();
		}
		else {
			this.valueDisplay.hide();
			this.valueDisplayNonInteractive.show();
		}
	}

	setValue(value) {
		this.valueDisplay.text(value);
		this.valueDisplayNonInteractive.text(value);
	}

	pushValue() {
		let value = eval(this.editValue.val());
		this.onSetValue(value);
		this.valueDisplay.text(value);
	}

	validate() {
		try {
			let result = eval(this.editValue.val());
			if(isNaN(result)) {
				throw("Result is not a number");
			}
			if(this.customValidateFunction) {
				this.customValidateFunction(result);
			}

			this.editValueError.hide();
			this.editValue.addClass('is-valid');
			this.editValue.removeClass('is-invalid');

			if(result == Math.floor(result)) {
				if(result < 0) {
					this.extraFeedback.text("-0x" + Math.abs(result.toString(16)));
					this.extraFeedback.show();
				}
				else {
					this.extraFeedback.text("0x" + result.toString(16));
				}
				this.extraFeedback.show();
			}
			return true;
		}
		catch(exception) {
			this.extraFeedback.hide();
			this.editValueError.show();
			this.editValueError.text(exception);
			this.editValue.removeClass('is-valid');
			this.editValue.addClass('is-invalid');
			return false;
		}
	}
}

export default EditableValue;