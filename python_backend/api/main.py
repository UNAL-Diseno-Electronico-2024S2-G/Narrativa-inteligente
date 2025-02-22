from prompts import *

from langchain.schema.output_parser import StrOutputParser
from langchain.schema.runnable import RunnableBranch

from langchain_openai import ChatOpenAI
from deep_translator import GoogleTranslator
from gtts import gTTS

import base64

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

# # Cargar los datos
# user_data_dict = load_user_data()

def get_b64_image(image_path):
    with open(image_path, "rb") as image_file:
        return base64.b64encode(image_file.read()).decode("utf-8")

def text_to_voice(text, fout="output.mp3"):
    tts = gTTS(text=text, lang='es')
    tts.save(fout)

# image_path = "/home/xhapa/Documents/PROGRAMMING/Projects/Narrativa-inteligente/python_backend/api/images/museos-mas-importantes-del-mundo.jpg"  # Update with your local image path
# image_data = get_b64_image(image_path)

# llm = ChatOpenAI(model='llava', base_url="http://localhost:11434/v1", api_key="ollama")

# prompt_template = format_prompts(user_data_dict)
# message = format_human_msg(user_data_dict, image_data, "image")

# chain = prompt_template | llm

# ai_msg = chain.invoke({"msgs": [message]})
# print(ai_msg.content)

# translated = GoogleTranslator(source='auto', target='es').translate(ai_msg.content)
# text_to_voice(translated)


def main():
    # Cargar los datos
    user_data_dict = load_user_data()

    image_path = "/home/xhapa/Documents/PROGRAMMING/Projects/Narrativa-inteligente/python_backend/api/images/museos-mas-importantes-del-mundo.jpg"  # Update with your local image path
    image_data = get_b64_image(image_path)

    llm = ChatOpenAI(model='llava', base_url="http://localhost:11434/v1", api_key="ollama")

    classification_template = format_classification(image_data)
    # urban_img_template = format_urban_prompts(user_data_dict, image_data)
    # nature_img_template = format_nature_prompts(user_data_dict, image_data)
    # indoor_img_template = format_indoor_prompts(user_data_dict, image_data)
    # historical_img_template = format_historical_prompts(user_data_dict, image_data)
    

    # branches = RunnableBranch(
    # (
    #     lambda x: "urban" in x,
    #     urban_img_template | llm
    # ),
    # (
    #     lambda x: "nature" in x,
    #     nature_img_template | llm
    # (
    #     lambda x: "indoor" in x,
    #     indoor_img_template | llm
    # ),
    # historical_img_template | llm
    # )
    # )

    classification_chain = classification_template | llm | StrOutputParser()

    # chain = classification_chain | branches

    # print("DONE")

    ai_msg = classification_chain.invoke()
    print(ai_msg.content)

    translated = GoogleTranslator(source='auto', target='es').translate(ai_msg.content)
    text_to_voice(translated)

if __name__ == "__main__":
    main()