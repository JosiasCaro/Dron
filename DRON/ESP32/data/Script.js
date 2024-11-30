var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getValues(){
    websocket.send("getValues");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    getValues();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function updateSliderPWM(element) {
    var sliderNumber = element.id.charAt(element.id.length-1);
    var sliderValue = document.getElementById(element.id).value;
    document.getElementById("sliderValue"+sliderNumber).innerHTML = sliderValue;
    console.log(sliderValue);
    websocket.send(sliderNumber+"s"+sliderValue.toString());
}

function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);

    for (var i = 0; i < keys.length; i++){
        var key = keys[i];
        document.getElementById(key).innerHTML = myObj[key];
        document.getElementById("slider"+ (i+1).toString()).value = myObj[key];
    }
}

//MPU-6050

let scene, camera, rendered, cube;

function parentWidth(elem) {
return elem.parentElement.clientWidth;
}

function parentHeight(elem) {
return elem.parentElement.clientHeight;
}

function init3D(){
scene = new THREE.Scene();
scene.background = new THREE.Color(0xffffff);

camera = new THREE.PerspectiveCamera(75, parentWidth(document.getElementById("3Dcube")) / parentHeight(document.getElementById("3Dcube")), 0.1, 1000);

renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setSize(parentWidth(document.getElementById("3Dcube")), parentHeight(document.getElementById("3Dcube")));

document.getElementById('3Dcube').appendChild(renderer.domElement);

// Create a geometry
const geometry = new THREE.BoxGeometry(5, 1, 4);

// Materials of each face
var cubeMaterials = [
new THREE.MeshBasicMaterial({color:0x03045e}),
new THREE.MeshBasicMaterial({color:0x023e8a}),
new THREE.MeshBasicMaterial({color:0x0077b6}),
new THREE.MeshBasicMaterial({color:0x03045e}),
new THREE.MeshBasicMaterial({color:0x023e8a}),
new THREE.MeshBasicMaterial({color:0x0077b6}),
];

const material = new THREE.MeshFaceMaterial(cubeMaterials);

cube = new THREE.Mesh(geometry, material);
scene.add(cube);
camera.position.z = 5;
renderer.render(scene, camera);
}

// Resize the 3D object when the browser window changes size
function onWindowResize(){
camera.aspect = parentWidth(document.getElementById("3Dcube")) / parentHeight(document.getElementById("3Dcube"));
//camera.aspect = window.innerWidth /  window.innerHeight;
camera.updateProjectionMatrix();
//renderer.setSize(window.innerWidth, window.innerHeight);
renderer.setSize(parentWidth(document.getElementById("3Dcube")), parentHeight(document.getElementById("3Dcube")));

}

window.addEventListener('resize', onWindowResize, false);

// Create the 3D representation
init3D();

// Create events for the sensor readings
if (!!window.EventSource) {
var source = new EventSource('/events');

source.addEventListener('open', function(e) {
console.log("Events Connected");
}, false);

source.addEventListener('error', function(e) {
if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
}
}, false);

source.addEventListener('gyro_readings', function(e) {
//console.log("gyro_readings", e.data);
var obj = JSON.parse(e.data);
document.getElementById("gyroX").innerHTML = obj.gyroX;
document.getElementById("gyroY").innerHTML = obj.gyroY;
document.getElementById("gyroZ").innerHTML = obj.gyroZ;

// Change cube rotation after receiving the readinds
cube.rotation.x = obj.gyroY;
cube.rotation.z = obj.gyroX;
cube.rotation.y = obj.gyroZ;
renderer.render(scene, camera);
}, false);

source.addEventListener('temperature_reading', function(e) {
console.log("temperature_reading", e.data);
document.getElementById("temp").innerHTML = e.data;
}, false);

source.addEventListener('accelerometer_readings', function(e) {
console.log("accelerometer_readings", e.data);
var obj = JSON.parse(e.data);
document.getElementById("accX").innerHTML = obj.accX;
document.getElementById("accY").innerHTML = obj.accY;
document.getElementById("accZ").innerHTML = obj.accZ;
}, false);

}

function resetPosition(element){
var xhr = new XMLHttpRequest();
xhr.open("GET", "/"+element.id, true);
console.log(element.id);
xhr.send();
}