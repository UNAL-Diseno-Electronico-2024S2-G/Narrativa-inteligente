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
    # # Cargar los datos
    user_data_dict = load_user_data()

    file_name = 'output.wav'
    image_path = "/home/xhapa/Documents/PROGRAMMING/Projects/Narrativa-inteligente/python_backend/api/images/photo.jpg"  # Update with your local image path
    image_data = get_b64_image(image_path)

    llm = ChatOpenAI(model='llava', base_url="http://localhost:11434/v1", api_key="ollama")

    prompt_template = format_prompts(user_data_dict)
    message = format_human_msg(user_data_dict, image_data)

    chain = prompt_template | llm

    ai_msg = chain.invoke({"msgs": [message]})
    # print(ai_msg.content)

    translated = GoogleTranslator(source='auto', target='es').translate(ai_msg.content)
    text_to_voice(translated, file_name)
