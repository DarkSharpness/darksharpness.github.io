"use strict";

document.addEventListener('DOMContentLoaded', () => {
    const STORAGE_KEY = 'my_blur_state';
    const webBg = document.getElementById('web_bg');
    if (!webBg) return;

    const hasSavedState = () => {
        return localStorage.getItem(STORAGE_KEY) !== null;
    };
    const on = () => {
        const savedState = localStorage.getItem(STORAGE_KEY);
        return savedState === null ? true : savedState === 'on';
    };
    const set = (on) => {
        localStorage.setItem(STORAGE_KEY, on ? 'on' : 'off');
    };
    const update = () => {
        webBg.style.filter = `blur(${on() ? 25 : 0}px)`;
    };
    const toggle = () => {
        set(!on());
        update();
    };
    const bindToggle = () => {
        const button = document.getElementById('blur_toggle');
        if (!button || button.dataset.blurBound === 'true') return;
        button.addEventListener('click', toggle, false);
        button.dataset.blurBound = 'true';
    };

    // Default to blur on for first-time visitors, and preserve saved preference afterwards.
    webBg.style.animation = 'none'; // Disable the animation.
    webBg.style.transition = 'filter 1s ease-out';
    if (!hasSavedState()) set(true);
    update();

    bindToggle();
    document.addEventListener('pjax:complete', () => {
        bindToggle();
        update();
    });
});
