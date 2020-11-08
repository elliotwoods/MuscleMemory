let _onClose = null;
let _onSave = null;
let _closeOnSave = false;

function showModal(title, onClose, onSave, closeOnSave=true) {
	$("#modal").modal('show');
	$("#modal_title").text(title);
	
	let modalBody = $("#modal_body");
	modalBody.empty();

	_onClose = onClose;
	_onSave = onSave;
	_closeOnSave = closeOnSave;

	if(onSave) {
		$("#modal_save").show();
	}
	else {
		$("#modal_save").hide();
	}

	return modalBody;
}

function onHideModal() {
	if(_onClose) {
		_onClose();
	}
	_onClose = null;
	_onSave = null;
}

function onSaveModal() {
	if(_onSave) {
		_onSave();
	}
}

$("#modal").on('hide.bs.modal', (e) => {
	onHideModal();
});

$("#modal_save").click(() => {
	onSaveModal();
});

export default showModal;