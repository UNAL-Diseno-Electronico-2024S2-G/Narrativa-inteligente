from flask import Flask, request, jsonify, send_file
import os
import speech_recognition as sr

app = Flask(__name__)
file_name = 'output.wav'
file_path = os.path.abspath(file_name)

UPLOAD_FOLDER = "/home/xhapa/Documents/PROGRAMMING/Projects/Narrativa-inteligente/python_backend/api/images"
os.makedirs(UPLOAD_FOLDER, exist_ok=True)  # Asegura que la carpeta exista

@app.route('/uploadAudio', methods=['POST'])
def upload_audio():
    if request.method == 'POST':
        try:
            # Save the uploaded file
            with open(file_name, 'wb') as f:
                f.write(request.data)
            
            # Transcribe the audio file
            transcription = speech_to_text(file_name)

            return jsonify({'transcription': transcription}), 200
        except Exception as e:
            return str(e), 500
    else:
        return 'Method Not Allowed', 405

@app.route('/uploadPhoto', methods=['POST'])
def upload_photo():
    if 'file' not in request.files:
        return "No file received", 400

    file = request.files['file']
    file_path = os.path.join(UPLOAD_FOLDER, "photo.jpg")
    file.save(file_path)

    return "File uploaded successfully", 200

@app.route('/downloadAudio', methods=['GET'])
def download_audio():
    try:
        # Verifica si el archivo existe
        if not os.path.exists(file_name):
            return "El archivo no existe", 404
        
        # Envía el archivo "recording.wav" para su descarga utilizando la ruta absoluta
        return send_file(file_path, mimetype='audio/wav', as_attachment=True)
    except Exception as e:
        return str(e), 500


def speech_to_text(file_name):
    # Inicializar el recognizer
    recognizer = sr.Recognizer()
    
    # Abrir el archivo de audio
    with sr.AudioFile(file_name) as source:
        # Escuchar y cargar el audio en memoria
        audio_data = recognizer.record(source)
        
        # Reconocer (convertir de voz a texto) especificando el idioma español
        try:
            text = recognizer.recognize_google(audio_data, language="es-ES")
            print(f'Transcripción: {text}')
            return text
        except sr.UnknownValueError:
            return "Google Speech Recognition no pudo entender el audio"
        except sr.RequestError as e:
            return f"No se pudieron solicitar resultados del servicio de Google Speech Recognition; {e}"


if __name__ == '__main__':
    port = 8888
    app.run(host='0.0.0.0', port=port)
    print(f'Listening at {port}')