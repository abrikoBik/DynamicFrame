document.addEventListener("DOMContentLoaded", function(){
    checkSpace();
    curDelay();
});

async function curDelay() {
    const response = await fetch("/api/cur_delay");
    const cur_delay = await response.text();
    document.getElementById("curDelay").textContent = "Текущее время: " + cur_delay;
}

function load(input_element) {
    const file = input_element.files[0];
    if (!file) return;

    const img = new Image();
    img.onload = () => {
        const canvas = document.createElement('canvas');
        canvas.width = 240;
        canvas.height = 320;
        const ctx = canvas.getContext('2d');
        
        const needsRotation = img.width > img.height;
        
        if (needsRotation) {
            console.log("Preview: Image is landscape, rotating 90° clockwise...");
            
            ctx.translate(canvas.width, 0);
            ctx.rotate(Math.PI / 2);
            
            const scale = Math.min(canvas.height / img.width, canvas.width / img.height);
            const drawWidth = img.width * scale;
            const drawHeight = img.height * scale;
            
            const offsetX = (canvas.height - drawWidth) / 2;
            const offsetY = (canvas.width - drawHeight) / 2;
            
            ctx.drawImage(img, offsetX, offsetY, drawWidth, drawHeight);
        } else {
            const scale = Math.min(canvas.width / img.width, canvas.height / img.height);
            const drawWidth = img.width * scale;
            const drawHeight = img.height * scale;
            
            const offsetX = (canvas.width - drawWidth) / 2;
            const offsetY = (canvas.height - drawHeight) / 2;
            
            ctx.drawImage(img, offsetX, offsetY, drawWidth, drawHeight);
        }
        
        const mimeType = file.type === 'image/png' ? 'image/png' : 'image/jpeg';
        document.getElementById('preview').src = canvas.toDataURL(mimeType);
    };
    
    img.src = URL.createObjectURL(file);
}

async function deleteFile(index) {
    await checkSpace();
    const responde = await fetch(`/api/files?index=${index}`, {
        method: "DELETE"
    });
    listFS();
}

async function listFS () {
    await checkSpace();
    const response = await fetch("/api/files");

    if(!response.ok) {
        console.error("HTTP error!");
        return;
    }

    const fs_json = await response.json();
    console.log(fs_json);

    const files_section = document.getElementById("files");
    files_section.replaceChildren();

    if(fs_json.files.length > 0) {
        fs_json.files.forEach((el, idx) => {
            const file_line = document.createElement("div");
            const delete_button = document.createElement("span");
            const preview_img = document.createElement("img");

            file_line.textContent = el;
            file_line.id = "file_line";
            files_section.appendChild(file_line);

            delete_button.textContent = "x";
            delete_button.id = "delete_button";
            delete_button.onclick = () => deleteFile(idx);
            file_line.appendChild(delete_button);

            preview_img.src = `/api/images?name=${el}`;
            preview_img.id = "file_prev";
            file_line.appendChild(preview_img);
        });
    } else {
        const file_line = document.createElement("div");
        file_line.textContent = "No files";
        files_section.appendChild(file_line);
    }
}

async function uploadFile() {
    await checkSpace();
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
            
            const needsRotation = img.width > img.height;
            
            if (needsRotation) {
                console.log("Image is landscape, rotating 90° clockwise...");
                
                ctx.translate(canvas.width, 0);
                ctx.rotate(Math.PI / 2);
                
                const scale = Math.min(canvas.height / img.width, canvas.width / img.height);
                const drawWidth = img.width * scale;
                const drawHeight = img.height * scale;
                
                const offsetX = (canvas.height - drawWidth) / 2;
                const offsetY = (canvas.width - drawHeight) / 2;
                
                ctx.drawImage(img, offsetX, offsetY, drawWidth, drawHeight);
            } else {
                const scale = Math.min(canvas.width / img.width, canvas.height / img.height);
                const drawWidth = img.width * scale;
                const drawHeight = img.height * scale;
                
                const offsetX = (canvas.width - drawWidth) / 2;
                const offsetY = (canvas.height - drawHeight) / 2;
                
                ctx.drawImage(img, offsetX, offsetY, drawWidth, drawHeight);
            }

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

async function changeDelay() {
    const data = new FormData();
    const delay = document.getElementById("delayPicker").value;
    data.append("delay", delay);

    document.getElementById("curDelay").textContent = "Текущее время: " + delay;

    const response = await fetch("/api/images/delay", {
        method: "POST",
        body: data
    });
}

async function checkSpace() {
    const response = await fetch("/api/space_used"); 
    const textData = await response.text();

    console.log(textData);

    const spaceUsedBytes = parseInt(textData.trim(), 10);

    if (isNaN(spaceUsedBytes)) {
        throw new Error(`Сервер вернул не число: "${textData}"`);
    }

    const spaceInKb = (spaceUsedBytes / 1000).toFixed(1); 

    // 5. Обновляем DOM
    const el = document.getElementById("curSpace");
    if (el) {
        el.textContent = `${spaceInKb}/1442 kB`;
    } else {
        console.warn('Элемент с id "curSpace" не найден');
    }
}