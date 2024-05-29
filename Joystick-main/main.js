const canvas = document.createElement('canvas'), context = canvas.getContext('2d');
document.body.append(canvas);

let width = canvas.width = innerWidth;
let height = canvas.height = innerHeight;

const FPS = 120;

class Boton{
    constructor(x,y,id){
        this.pos = new Vector2(x, y);
        this.radius = 20;
        this.id = id;
        this.createButtonElement();
    }
    createButtonElement() {
        // Crear un elemento HTML para el botón
        this.element = document.createElement('div');
        this.element.style.position = 'absolute';
        this.element.style.left = `${this.pos.x - this.radius}px`;
        this.element.style.top = `${this.pos.y - this.radius}px`;
        this.element.style.width = `${this.radius * 2}px`;
        this.element.style.height = `${this.radius * 2}px`;
        this.element.style.borderRadius = '50%';
        this.element.style.backgroundColor = 'red';
        this.element.style.zIndex = '1000'; // Asegura que el botón esté al frente
        this.element.id = this.id;

        // Agregar el elemento al documento
        document.body.appendChild(this.element);

        // Agregar los event listeners al elemento
        this.element.addEventListener('touchstart', this.touchStartHandler.bind(this));
        this.element.addEventListener('click', this.clickHandler.bind(this));
    }

    touchStartHandler(e) {
        if (e) console.log(`Touch en botón ${this.id}`);
    }

    clickHandler(e) {
        if (e) console.log(`Click en botón ${this.id}`);
    }
}

function background() {
    context.fillStyle = '#000';
    context.fillRect(0, 0, width, height);
}

let joysticks = [
    new Joystick(80, height - 110, 50, 25)
];
let botones = [
    // boton abajo (A)
    new Boton(1150, height - 70,"A"),
    // boton derecha (B)
    new Boton(1200, height - 110,"B"),
    // boton izquierda (X)
    new Boton(1100, height - 110,"X"),
    // boton arriba (Y)
    new Boton(1150, height - 150,"Y")
    ];

setInterval(() => {
    background();

    for (let joystick of joysticks) {
        joystick.update();
    }

    
    

}, 1000 / FPS);
