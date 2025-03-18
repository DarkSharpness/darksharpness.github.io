"use strict";

document.addEventListener('DOMContentLoaded', () => {
    const webBg = document.getElementById('web_bg');

    const on = () => localStorage.getItem('my_blur_state') === 'on';
    const set = (on) => localStorage.setItem('my_blur_state', on ? 'on' : 'off');
    const update = () => {
        webBg.style.filter = `blur(${on() ? 25 : 0}px)`;
    };
    const toggle = () => {
        set(!on());
        update();
    };

    // Initialize the state.
    webBg.style.animation = 'none'; // Disable the animation.
    webBg.style.transition = 'filter 1s ease-out';
    set(on());
    update();

    document.getElementById('blur_toggle')?.addEventListener('click', toggle, false);
    document.addEventListener('pjax:complete', () => {
        document.getElementById('blur_toggle')?.addEventListener('click', toggle, false);
    });
});
