import ollama
import time
from PIL import Image
import io

from gtts import gTTS
from deep_translator import GoogleTranslator

def get_b_array(image_path):
    with open(image_path, 'rb') as image_file:
        img1 = Image.open(image_file)
        
        # Convert the image to a byte stream in JPEG format
        img_byte_arr = io.BytesIO()
        img1.save(img_byte_arr, format='JPEG')  # or 'PNG' if you prefer
        img_byte_arr = img_byte_arr.getvalue()
    return img_byte_arr

def text_to_voice(text, fout = "output.mp3"):
    tts = gTTS(text=text, lang='es')
    tts.save(fout)

img_path = '/home/xhapa/Documents/PROGRAMMING/Projects/Narrativa-inteligente/python_backend/api/images/picture8.jpg'

image_bytearray = get_b_array(img_path) #bytearray(buffer)
print('responding')
response = ollama.chat(model='llava', messages=[
{
    'role': 'user',
    "prompt":"How many peoplare in the image?",
    "images": [image_bytearray]
    },
])
print('responded')
text = (response['message'])['content']
print(text)

translated = GoogleTranslator(source='auto', target='es').translate(text)

text_to_voice(translated)