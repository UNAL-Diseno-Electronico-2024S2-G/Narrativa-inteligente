import os
import speech_recognition as sr
from threading import Thread  # Importa Thread para ejecutar tareas en segundo plano
import subprocess
import time
from flask import Flask, request, jsonify, send_file, redirect, Response
import requests

# Importa la función api desde el archivo correspondiente
from api.api_utils import api

app = Flask(__name__)
file_name = 'output.wav'
file_path = os.path.abspath(file_name)

UPLOAD_FOLDER = "/home/xhapa/Documents/PROGRAMMING/Projects/Narrativa-inteligente/python_backend/api/images"
os.makedirs(UPLOAD_FOLDER, exist_ok=True)  # Asegura que la carpeta exista

# Variable para rastrear si la generación de audio está en progreso
is_generating_audio = False

@app.route('/uploadAudio', methods=['POST'])
def upload_audio():
    if request.method == 'POST':
        try:
            # Guardar el archivo subido
            with open(file_name, 'wb') as f:
                f.write(request.data)
            
            # Transcribir el archivo de audio
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
    global is_generating_audio

    try:
        # Si ya se está generando el audio, informa al usuario
        if is_generating_audio:
            return jsonify({"status": "processing", "message": "El audio se está generando. Por favor, inténtalo de nuevo más tarde."}), 202

        # Marca que la generación de audio ha comenzado
        is_generating_audio = True

        # Ejecuta api() en un hilo separado
        thread = Thread(target=api)
        thread.start()

        return jsonify({"status": "started", "message": "La generación de audio ha comenzado. Por favor, inténtalo de nuevo en unos minutos."}), 202
    except Exception as e:
        is_generating_audio = False  # Reinicia el estado en caso de error
        return str(e), 500

@app.route('/checkAudio', methods=['GET'])
def check_audio():
    global is_generating_audio

    try:
        # Verifica si el archivo de audio ya está listo
        if os.path.exists(file_name):
            is_generating_audio = False  # Reinicia el estado
            return send_file(file_path, mimetype='audio/wav', as_attachment=True)
        else:
            return jsonify({"status": "processing", "message": "El audio aún se está generando."}), 202
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

# Variables para Streamlit
streamlit_port = 8501  # Puerto predeterminado de Streamlit
streamlit_process = None
streamlit_ready = False

def start_streamlit():
    """Función para iniciar el proceso de Streamlit"""
    global streamlit_process, streamlit_ready
    
    # Ruta al archivo de Streamlit (ajusta según tu estructura de directorios)
    streamlit_file = "/home/xhapa/Documents/PROGRAMMING/Projects/Narrativa-inteligente/python_backend/app/main.py"
    
    # Iniciar el proceso de Streamlit
    streamlit_process = subprocess.Popen(
        ["streamlit", "run", streamlit_file, "--server.port", str(streamlit_port)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    
    # Esperar a que Streamlit esté listo (buscar mensaje en stdout)
    while True:
        line = streamlit_process.stdout.readline().decode('utf-8')
        if "You can now view your Streamlit app in your browser" in line:
            streamlit_ready = True
            break
        if not line:
            break
        time.sleep(0.1)
    
    print(f"Streamlit está ejecutándose en el puerto {streamlit_port}")

# Iniciar Streamlit en un hilo separado
Thread(target=start_streamlit).start()

@app.route('/streamlit', methods=['GET'])
def redirect_to_streamlit():
    """Redirige al usuario a la aplicación de Streamlit"""
    if not streamlit_ready:
        return "La aplicación Streamlit se está inicializando, por favor espere...", 503
    return redirect(f"http://localhost:{streamlit_port}")

@app.route('/streamlit/<path:path>', methods=['GET', 'POST', 'PUT', 'DELETE'])
def proxy_streamlit(path):
    """Funciona como un proxy para las solicitudes a Streamlit"""
    if not streamlit_ready:
        return "La aplicación Streamlit se está inicializando, por favor espere...", 503
    
    # Construir la URL de destino
    target_url = f"http://localhost:{streamlit_port}/{path}"
    
    # Enviar la solicitud al servidor de Streamlit
    resp = requests.request(
        method=request.method,
        url=target_url,
        headers={key: value for key, value in request.headers if key != 'Host'},
        data=request.get_data(),
        cookies=request.cookies,
        allow_redirects=False,
        stream=True
    )
    
    # Crear respuesta
    excluded_headers = ['content-encoding', 'content-length', 'transfer-encoding', 'connection']
    headers = [(name, value) for name, value in resp.raw.headers.items()
               if name.lower() not in excluded_headers]
    
    response = Response(resp.content, resp.status_code, headers)
    return response

# Al finalizar el servidor, cerrar el proceso de Streamlit
import atexit
@atexit.register
def cleanup():
    if streamlit_process:
        streamlit_process.terminate()
        print("Proceso de Streamlit terminado")


if __name__ == '__main__':
    port = 8888
    app.run(host='0.0.0.0', port=port)
    print(f'Listening at {port}')