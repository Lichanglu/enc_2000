/*
* ENC1260������
*/

function initPageMenu() {
	//������۳�ʼ��
	roller.init('wm-main-top', 'v', -42, 0, 143, 20, 3);
	//������Ϊ��ʼ��
	initRollerAction();
	
}

function showLeftMenuToSysInfo() {
	jQuery('#menu').hide();
	jQuery('#menu-right').css({
		"width": "965px",
		"margin-left": "15px"
	});
	jQuery('#maincontent').css({
		"width": "920px",
		"text-align": "center"
	});
}

function showLeftMenuFromSysInfo() {
	jQuery('#menu').show();
	jQuery('#menu-right').css({
		"width": "805px",
		"margin-left": "-2px"
	});
	jQuery('#maincontent').css({
		"width": "770px",
		"text-align": "left"
	});
}

function showleftMenuAnimate() {
	jQuery('#nav').hide();
	jQuery('#nav').show(500);
}

function showMainContentAnimate() {
	jQuery('#menu-right').animate({width:"toggle"}, 0);
	jQuery('#menu-right').animate({width:"toggle"}, 500);
}

function initRollerAction() {
	//����ϵͳ��Ϣҳ��
	jQuery('#nav_sysinfo').click(function() {
		if(!window.sysinfoRefreshTimestamp || (new Date().getTime() - window.sysinfoRefreshTimestamp) > 500) {
			window.sysinfoRefreshTimestamp = new Date().getTime();
			closeAllPrompt();
			jQuery('#wmform').html('');
			//roller.firstClicked = true;
			if((new Date().getTime()-roller.lastClickedTime) < 500) {
				return false;
			}
			roller.lastClickedTime = new Date().getTime();
			jQueryAjaxHtml({
				data: {"actioncode": "303"}
			});
			showLeftMenuToSysInfo();
			//showMainContentAnimate();
		} else {
			return false;
		}
	});	
	
	//���ز������ò˵�
	jQuery('#nav_paramset').click(function() {
		//roller.firstClicked = true;
		if((new Date().getTime()-roller.lastClickedTime) < 500) {
			return false;
		}
		roller.lastClickedTime = new Date().getTime();
		jQueryAjaxHtml({
			data: {"actioncode": "301"},
			renderToID: 'nav',
		});
		showLeftMenuFromSysInfo();
		showleftMenuAnimate();
	});
	
	//����ϵͳ���ò˵�
	jQuery('#nav_sysset').click(function() {
		//roller.firstClicked = true;
		if((new Date().getTime()-roller.lastClickedTime) < 500) {
			return false;
		}
		roller.lastClickedTime = new Date().getTime();
		jQueryAjaxHtml({
			data: {"actioncode": "302"},
			renderToID: 'nav'
		});
		showLeftMenuFromSysInfo();
		showleftMenuAnimate();
	});
	
	//�����һ������
	jQuery('#nav_paramset').click();
}

/*
 * ��ʼ��͸���Ȼ�����
 * */
function initBrightnessSlider() {
	window.cap_sliderbar = new Slider("cap_bright_slider", "cap_bright_bar", {
		onMove: function(){
			if(navigator.userAgent.indexOf("MSIE 8.0")>0) {
				O("cap_brightness").value = Math.round(this.GetValue()) - 1;
			} else {
				O("cap_brightness").value = Math.round(this.GetValue());
			}
			
		}
	});
	
	jQuery('#cap_brightness').change(function() {
		var value = O("cap_brightness").value;
		if(isNaN(value)) {
			value = 100;
		}
		cap_sliderbar.SetValue(parseInt(value, 10));
	});
	
	window.logo_sliderbar = new Slider("logo_bright_slider", "logo_bright_bar", {
		onMove: function(){
			if(navigator.userAgent.indexOf("MSIE 8.0")>0) {
				O("logo_brightness").value = Math.round(this.GetValue()) - 1;
			} else {
				O("logo_brightness").value = Math.round(this.GetValue());
			}
			
		}
	});
	jQuery('#logo_brightness').change(function() {
		var value = O("logo_brightness").value;
		if(isNaN(value)) {
			value = 100;
		}
		logo_sliderbar.SetValue(parseInt(value, 10));
	});
	
}

/*
 * ��������ֵ�󻬶���û����������
 */
function fixBrightnessSlider() {
	var value = O("cap_brightness").value;
	if(isNaN(value)) {
		value = 100;
	}
	cap_sliderbar.SetValue(parseInt(value, 10));
	
	var value = O("logo_brightness").value;
	if(isNaN(value)) {
		value = 100;
	}
	logo_sliderbar.SetValue(parseInt(value, 10));
}

/*
 * ������
 * */
function formBeautify() {
	zebraTransform.update();
}


/*
 * ��ʼ��̧ͷ��Ϣ
 * */
function initTopInfo() {
	var loginUserName = jQuery.cookies.get('user');
	if(loginUserName) {
		jQuery('#loginUserName').html(loginUserName);
	}
	jQuery('#logoutLink').click(function() {
		jQueryAjaxCmd({
			data: {"actioncode": "200"},
			success: function(ret) {
				eval(ret);
			}
		});
	});
}


/*
 * ��ʾ������uploading��ʾ��
 */
function openUploading() {
	jQuery('#uploadingDiv').width(document.body.scrollWidth);
	jQuery('#uploadingDiv').height(document.body.scrollHeight);
	jQuery('#uploadingDiv').show();
}

function closeUploading() {
	jQuery('#uploadingDiv').hide();
}




