from .prompts import *

from langchain.schema.output_parser import StrOutputParser
from langchain.schema.runnable import RunnableBranch

from langchain_openai import ChatOpenAI
from deep_translator import GoogleTranslator
from gtts import gTTS
import torchaudio
import torchaudio.transforms as transforms
import torch

import base64

import os
from datetime import datetime

import json
 
# Función para cargar los datos desde el archivo JSON
def load_user_data(filename="/home/xhapa/Documents/PROGRAMMING/Projects/Narrativa-inteligente/python_backend/app/user_data.json"):
    try:
        with open(filename, "r") as f:
            user_data = json.load(f)  # Cargar el JSON como diccionario
        return user_data
    except FileNotFoundError:
        print("Archivo no encontrado. Asegúrate de guardar los datos primero.")
        return {}
    except json.JSONDecodeError:
        print("Error al leer el archivo JSON. Puede estar corrupto.")
        return {}

def get_b64_image(image_path):
    with open(image_path, "rb") as image_file:
        return base64.b64encode(image_file.read()).decode("utf-8")

def text_to_voice(text, output_wav="output.wav", lang="es", sample_rate=24000):
    tts = gTTS(text=text, lang=lang)
    temp_mp3 = "temp.mp3"
    tts.save(temp_mp3)

    # Load MP3 using torchaudio
    waveform, orig_sample_rate = torchaudio.load(temp_mp3)

    # Resample if needed
    if orig_sample_rate != sample_rate:
        resampler = transforms.Resample(orig_sample_rate, sample_rate)
        waveform = resampler(waveform)

    # Save as WAV
    torchaudio.save(output_wav, waveform, sample_rate)
    return output_wav

def api():
    # Cargar los datos del usuario
    user_data_dict = load_user_data()
    
    # Rutas de los archivos
    image_path = "/home/xhapa/Documents/PROGRAMMING/Projects/Narrativa-inteligente/python_backend/server/photo.jpg"
    audio_path = "/home/xhapa/Documents/PROGRAMMING/Projects/Narrativa-inteligente/python_backend/server/output.wav"
    
    # Obtener timestamps de modificación de los archivos
    image_mtime = os.path.getmtime(image_path)
    audio_mtime = os.path.getmtime(audio_path)
    
    # Determinar el último archivo modificado
    last_modified_file = image_path if image_mtime > audio_mtime else audio_path
    
    # Inicializar LLM y variables
    llm = ChatOpenAI(model='llava', base_url="http://localhost:11434/v1", api_key="ollama")
    
    # Procesamiento basado en el último archivo modificado
    if last_modified_file == image_path:
        # Procesar imagen
        image_data = get_b64_image(image_path)
        
        # Cadena de procesamiento para imagen
        prompt_template = format_prompts(user_data_dict)
        message = format_human_msg(user_data_dict, image_data)
        chain = prompt_template | llm
        
        # Invocar análisis de imagen
        ai_msg = chain.invoke({"msgs": [message]})
        
    else:
        # Procesar audio
        # Cargar archivo de audio para transcripción
        with open(audio_path, 'rb') as audio_file:
            audio_data = audio_file.read()
        
        # Cadena de procesamiento para audio
        audio_chain = create_audio_analysis_chain(user_data_dict)
        
        # Invocar análisis de audio
        ai_msg = audio_chain.invoke({
            "audio": audio_data,
            "user_data": user_data_dict
        })
    
    # Traducir y convertir a voz (común para ambos casos)
    translated = GoogleTranslator(source='auto', target='es').translate(ai_msg.content)
    text_to_voice(translated, audio_path)
    
    # Registro de la operación
    print(f"Procesado: {last_modified_file}")
    print(f"Traducción final: {translated}")

# Ejemplo de una posible función para crear la cadena de análisis de audio
def create_audio_analysis_chain(user_data_dict):
    # Esta función necesitaría ser implementada
    # Podría usar un modelo de transcripción o análisis de audio
    audio_llm = ChatOpenAI(model='your-audio-model', base_url="http://localhost:11434/v1", api_key="ollama")
    
    # Crear una cadena de procesamiento específica para audio
    audio_prompt_template = ChatPromptTemplate.from_template("""
    Analiza el siguiente archivo de audio considerando los siguientes datos de usuario:
    {user_data}
    
    Contenido del audio: {audio}
    
    Proporciona un análisis detallado...
    """)
    
    return audio_prompt_template | audio_llm
