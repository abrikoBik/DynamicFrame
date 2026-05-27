// Загрузка файла на ESP32
async function uploadFile() {
    const fileInput = document.getElementById('fileInput');
    const statusDiv = document.getElementById('status');
    const file = fileInput.files[0];
    
    if (!file) {
        statusDiv.innerHTML = '<span style="color: red">Выберите файл!</span>';
        return;
    }
    
    // Проверка размера (опционально, например макс 500KB)
    const maxSize = 500 * 1024;
    if (file.size > maxSize) {
        statusDiv.innerHTML = '<span style="color: red">Файл слишком большой! Макс 500KB</span>';
        return;
    }
    
    // Создаем FormData
    const formData = new FormData();
    formData.append('file', file);
    
    statusDiv.innerHTML = 'Загрузка...';
    
    try {
        const response = await fetch('/upload', {
            method: 'POST',
            body: formData
        });
        
        if (response.ok) {
            statusDiv.innerHTML = '<span style="color: green">Успешно загружено!</span>';
            // Обновляем превью
            loadPreview(file);
        } else {
            statusDiv.innerHTML = '<span style="color: red">Ошибка загрузки!</span>';
        }
    } catch (error) {
        console.error('Error:', error);
        statusDiv.innerHTML = '<span style="color: red">Ошибка сети!</span>';
    }
}

// Показываем превью загруженного файла
function loadPreview(file) {
    const reader = new FileReader();
    reader.onload = function(e) {
        document.getElementById('preview').src = e.target.result;
    };
    reader.readAsDataURL(file);
}

// Существующая функция load() для просмотра файлов из LittleFS
function load() {
    const input = document.getElementById('picker');
    const file = input.files[0];
    if (!file) return;

    const img = new Image();
    img.onload = () => {
        const canvas = document.createElement('canvas');
        canvas.width = 320;
        canvas.height = 240;
        const ctx = canvas.getContext('2d');
        ctx.drawImage(img, 0, 0, 320, 240);
        
        const mimeType = file.type === 'image/png' ? 'image/png' : 'image/jpeg';
        document.getElementById('preview').src = canvas.toDataURL(mimeType);
    };
    
    img.src = URL.createObjectURL(file);
}