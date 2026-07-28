const select = (()=> {
    const cached = new Map();
    return query => {
        if(cached.has(query)) return cached.get(query);
        const el = document.querySelector(query);
        cached.set(query, el);
        return el;
    };
})()

let toggleMenu, togglePause;

const opt = (() => {
    const defaults = {
        menu: true, 
        paused: false, 
        toggle: false, 
        color_mode: false,
        mirror: true,
        invert: false,
    
        charset_input:{
            type: "text", 
            default: "@%#*+=-:. "
        }, 
        vision: {
            type : "range", 
            default: 25, 
            min: 1, 
            max: 100,
            step: 1,  
        },
        columns: {
            type : "range", 
            default: 100, 
            min: 30, 
            max: 200,
            step: 1,  
        },
        gamma: {
            type : "range", 
            default: 1.2, 
            min: 0.5, 
            max: 3,
            step: 0.1,  
        },
        charset: {
            type : "select", 
            default: "default", 
            options: [
                "blocks", 
                "minimal",
                "symbols",
                "default",
            ]
        },
        contrast: {
            type : "range", 
            default: 100, 
            min: 0.5, 
            max: 4,
            step: 0.1,
        },
        palette:{
            type: "palette",
            default: [
                {//very dark
                    threshold: 0.15, 
                    color : "#2a1a4a",

                },
                { //shadows
                    threshold: 0.30, 
                    color : "#6a2faf",

                },
                { //mid-tones
                    threshold: 0.50, 
                    color : "#c87fff",

                },
                { //bright
                    threshold: 0.70, 
                    color : "#ff9fe8",

                },
                { //highlights
                    threshold: 0.85, 
                    color : "#ffcfa0",

                },
                { //brightest
                    threshold: 1.00, 
                    color : "#ffffff",

                },

            ]
        }


    }
})();