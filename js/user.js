document.addEventListener('DOMContentLoaded', function () {
    const webBg     = document.getElementById('web_bg');
    const address   = 'my_blur_state'; // The address of the item.

    // Read the current state from local storage.
    let current = localStorage.getItem(address);
    let button  = document.getElementById('blur_toggle');

    function toggle() {
        if (current === 'off') {
            current = 'on';
            webBg.style.filter = 'blur(7px)';
        } else {
            current = 'off';
            webBg.style.filter = 'blur(0px)';
        }
        localStorage.setItem(address, current);
    }

    function set_listener() {
        button = document.getElementById('blur_toggle');
        if (button) {
            button.addEventListener('click', toggle, false);
        }
    }

    function global_init() {
        // If not set, set the default value.
        if (current === null) {
            current = 'off'; // Default off.
            localStorage.setItem(address, current);
        }

        // Load for the first time
        if (current === 'on') {
            webBg.style.filter = 'blur(0px)';
            webBg.style.filter = 'blur(7px)';
        }

        // Set the first listener.
        set_listener();
    }

    global_init();
    document.addEventListener('pjax:complete', set_listener);
});