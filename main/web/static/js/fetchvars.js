var root = {};
const mappings = [4, 3, 2, 1, 8, 7, 6, 5, 12, 11, 10, 9, 16, 15, 14, 13];
var esp_discovered = [];

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

function toBinaryString(num, length) {
    let binary = num.toString(2);
    let zerosToAdd = length - binary.length;
    let zeroString = '0'.repeat(zerosToAdd);
    return zeroString + binary;
}

function convertInputs(inputs) {
    let data1 = toBinaryString(inputs[0], 8);
    let data2 = toBinaryString(inputs[1], 8);

    let data = data1 + data2;
    let newData = '0000000000000000'.split('');
    let x = 0;

    for (let mapping of mappings) {
        mapping -= 1;
        newData[mapping] = data[x];
        x += 1;
    }

    data = newData.join('');
    x = 1;
    let channelsConnectedNew = [];

    for (let bit of data) {
        if (bit === '1') {
            channelsConnectedNew.push(x);
        }
        x += 1;
    }

    return channelsConnectedNew;
}

function update_esp_data(launcher, data) {
    channels_connected = convertInputs(data.inputData);
    for (let i = 0; i < data.discovered.length; i++) {
        if (!(esp_discovered.includes(data.discovered[i])) && !(data.discovered[i] == window.location.hostname)) {
            count = 16
            new_launcher = data.discovered[i];
            esp_discovered.push(new_launcher);
            fireworks_launched[new_launcher] = [];
            root.firework_profiles[new_launcher] = makeNewProfile(count);
            root.launcher_data.armed[new_launcher] = false;
            root.launcher_data.channels_connected[new_launcher] = [];
            root.launcher_data.counts[new_launcher] = count;
            root.launcher_data.names[new_launcher] = "Node " + new_launcher.split('.')[3];
            root.launchers.push(new_launcher);
            root.discovered.push(new_launcher);
        }
    }
    root.launcher_data.channels_connected[launcher] = channels_connected
}

function esp_fetch(launcher, body) {
    data = {
        method: "post",
        headers: {
            "Content-Type": "application/json"
        },
        body: body
    }
    endpoint = "/run_command"
    fetch("http://" + launcher + endpoint, data)
        .then(data => data.json())
        .then(json => update_esp_data(launcher, json));
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
    root["launcher_data"]["channels_connected"][window.location.hostname] = [];
    root["launcher_data"]["counts"][window.location.hostname] = count;
    root["launcher_data"]["names"][window.location.hostname] = "Node " + window.location.hostname.split('.')[3];
    root["launchers"] = [window.location.hostname];
    root["discovered"] = [];
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

function callEsp() {
    for (let i = 0; i < root.launchers.length; i++) {
        esp_fetch(root.launchers[i], JSON.stringify({"code": 0}));
    }
}

async function fetch_variables(script) {
    getDataFromSession();
    setInterval(saveData, 250);
    setInterval(callEsp, 3000);
    main_script = document.createElement("script");
    main_script.setAttribute("src", script);
    setTimeout(function() {
        document.body.appendChild(main_script);
    }, 50);
}