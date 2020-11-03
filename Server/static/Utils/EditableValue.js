class EditableValue {
	constructor(parent, defaultValue, onSetValue, validateFunction) {
		this.parent = parent;

		this.liveValue = $(`<a href="#"></a>`);
		this.parent.append(this.liveValue);
		this.liveValue.click(() => {
			this.openEditor();
		});

		
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

		this.editValueError = $(`<div class="invalid-feedback">Sorry, that username's taken. Try another?</div>`);
		this.parent.append(this.editValueError);
		this.editValueError.hide();

		this.setValue(defaultValue);

		this.customValidateFunction = validateFunction;
		this.onSetValue = onSetValue;
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

	setValue(value) {
		this.liveValue.text(value);
	}

	pushValue() {
		let value = eval(this.editValue.val());
		this.onSetValue(value);
		this.liveValue.text(value);
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
}

export default EditableValue;