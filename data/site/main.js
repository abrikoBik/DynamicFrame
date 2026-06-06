// Существующая функция load() для просмотра файлов из LittleFS
function load(input_element) {
    const file = input_element.files[0];
    if (!file) return;

    const img = new Image();
    img.onload = () => {
        const canvas = document.createElement('canvas');
        canvas.width = 240;
        canvas.height = 320;
        const ctx = canvas.getContext('2d');
        ctx.drawImage(img, 0, 0, 240, 320);
        
        const mimeType = file.type === 'image/png' ? 'image/png' : 'image/jpeg';
        document.getElementById('preview').src = canvas.toDataURL(mimeType);
    };
    
    img.src = URL.createObjectURL(file);
}

async function listFS () {
    const response = await fetch("/api/listFS");
    console.log(response);
}

async function uploadFile() {
    const input = document.getElementById("fileInput");
    const file = input.files[0];

    if(!file) {
        console.error("No file presented");
        return;
    }

    // resizing
    console.log("Resizing image...");
    const resizedBlob = await new Promise((resolve, reject) => {
        const img = new Image();
        img.onload = () => {
            const canvas = document.createElement("canvas");
            canvas.width = 240;
            canvas.height = 320;
            const ctx = canvas.getContext("2d");
            ctx.drawImage(img, 0, 0, 240, 320);

            canvas.toBlob((blob) => {
                resolve(blob);
            }, "image/jpeg", 0.7);
        }

        img.onerror = reject;
        img.src = URL.createObjectURL(file);
    });

    // uploading
    const data = new FormData();

    data.append('image', resizedBlob, file.name);

    console.log("Uploading...");
    const response = await fetch("/api/upload", {
        method: "POST",
        body: data
    });

    console.log(response);
}