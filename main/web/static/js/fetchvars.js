var root = {};

function makeNewProfile(count) {
    firework_list = []
    for (let i = 1; i < count+1; i++) {
        firework_list.push(i);
    }

    profile = {
        "1": {
            "color": "#177bed",
            "fireworks": firework_list,
            "name": "One Shot",
            "pwm": 1875
        },
        "2": {
            "color": "#5df482",
            "fireworks": [],
            "name": "Two Shot",
            "pwm": 3750
        },
        "3": {
            "color": "#f4ff5e",
            "fireworks": [],
            "name": "Three Shot",
            "pwm": 5625
        },
        "4": {
            "color": "#ff2667",
            "fireworks": [],
            "name": "Finale",
            "pwm": 3750
        }
    };
    return profile;
}

function esp_fetch(launcher, endpoint, data={}) {
    if (!data.headers) {
        data.headers = {};
    }
    data.headers["Content-Type"] = "application/json";
    return fetch("http://" + launcher + endpoint, data);
}

function generateNewData() {
    count = 16;
    root["drawflow_sequences"] = {};
    root["firework_profiles"] = {};
    root["firework_profiles"][window.location.hostname] = makeNewProfile(count);
    root["fireworks_launched"] = {};
    root["fireworks_launched"][window.location.hostname] = []
    root["labels"] = {};
    root["labels"][window.location.hostname] = [];
    root["launcher_data"] = {
        "armed": {},
        "channels_connected": {},
        "counts": {},
        "names": {}
    }
    root["launcher_data"]["armed"][window.location.hostname] = false;
    root["launcher_data"]["channels_connected"][window.location.hostname] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16];
    root["launcher_data"]["counts"][window.location.hostname] = count;
    root["launcher_data"]["names"][window.location.hostname] = "Node";
    root["launchers"] = [window.location.hostname];
    root["sequences"] = {};
}

function getDataFromSession() {
    const storedData = sessionStorage.getItem("launcherData");
    if (!(storedData == undefined || storedData == null || storedData == "undefined")) {
        root = JSON.parse(storedData);
    } else {
        generateNewData();
    }
}

function saveData() {
    if (!(JSON.stringify(root) == "{}")) {
        sessionStorage.setItem("launcherData", JSON.stringify(root));
    }
}

async function fetch_variables(script) {
    getDataFromSession();
    setInterval(saveData, 250);
    main_script = document.createElement("script");
    main_script.setAttribute("src", script);
    setTimeout(function() {
        document.body.appendChild(main_script);
    }, 50);
}
