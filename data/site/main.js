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

async function deleteFile(index) {
    const responde = await fetch(`/api/files?index=${index}`, {
        method: "DELETE"
    });
    listFS();
}

async function listFS () {
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