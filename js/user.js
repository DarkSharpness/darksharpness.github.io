document.addEventListener('DOMContentLoaded', function () {
    const webBg = document.getElementById('web_bg');
    let current = localStorage.getItem('blur_state');
    if (current === null) {
        current = 'on';
        localStorage.setItem('blur_state', current);
    }

    if (current === 'on') {
        webBg.style.filter = 'blur(7px)';
    }

    let button = document.getElementById('blur_toggle');

    function toggle() {
        if (current === 'off') {
            current = 'on';
            webBg.style.filter = 'blur(7px)';
        } else {
            current = 'off';
            webBg.style.filter = 'blur(0px)';
        }
        localStorage.setItem('blur_state', current);
    }

    function global_init() {
        button = document.getElementById('blur_toggle');
        if (button) {
            button.addEventListener('click', toggle, false);
        }
    }

    global_init(); // First, lets init!
    document.addEventListener('pjax:complete', global_init);
});