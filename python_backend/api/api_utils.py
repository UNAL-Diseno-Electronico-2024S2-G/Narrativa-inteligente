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

def load_user_data(filename="/home/xhapa/Documents/PROGRAMMING/Projects/Narrativa-inteligente/python_backend/app/user_data.json"):
    try:
        with open(filename, "r") as f:
            user_data = json.load(f)
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
    # Ensure directory exists
    os.makedirs(os.path.dirname(output_wav), exist_ok=True)
    
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
    
    # Clean up temporary MP3 file
    os.remove(temp_mp3)
    
    return output_wav

def ensure_audio_exists(audio_path):
    """
    Ensure an audio file exists. If not, create a placeholder audio file.
    """
    if not os.path.exists(audio_path):
        # Create a placeholder text and convert to audio
        placeholder_text = "Por favor, proporciona un archivo de audio o graba algo."
        text_to_voice(placeholder_text, audio_path)
    return audio_path

def write_generation_status(status):
    """Write generation status to a file"""
    status_file = "/home/xhapa/Documents/PROGRAMMING/Projects/Narrativa-inteligente/python_backend/server/generation_status.json"
    with open(status_file, 'w') as f:
        json.dump({"status": status}, f)

def api():
    try:
        # Mark start of generation
        write_generation_status("processing")
        
        # Cargar los datos del usuario
        user_data_dict = load_user_data()
        
        # Rutas de los archivos
        image_path = "/home/xhapa/Documents/PROGRAMMING/Projects/Narrativa-inteligente/python_backend/server/photo.jpg"
        audio_path = "/home/xhapa/Documents/PROGRAMMING/Projects/Narrativa-inteligente/python_backend/server/output.wav"
        
        # Ensure audio file exists
        ensure_audio_exists(audio_path)
        
        # Inicializar LLM y variables
        llm = ChatOpenAI(model='llava', base_url="http://localhost:11434/v1", api_key="ollama")
        
        # Procesar imagen si existe
        if os.path.exists(image_path):
            # Procesar imagen
            image_data = get_b64_image(image_path)
            
            # Cadena de procesamiento para imagen
            prompt_template = format_prompts(user_data_dict)
            message = format_human_msg(user_data_dict, image_data)
            chain = prompt_template | llm
            
            # Invocar análisis de imagen
            ai_msg = chain.invoke({"msgs": [message]})
        else:
            # Procesar audio si no hay imagen
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
        print(f"Procesado: {'Imagen' if os.path.exists(image_path) else 'Audio'}")
        print(f"Traducción final: {translated}")
        
        # Mark successful completion
        write_generation_status("completed")
    
    except Exception as e:
        # Mark generation as failed
        write_generation_status("failed")
        print(f"Error during generation: {e}")

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