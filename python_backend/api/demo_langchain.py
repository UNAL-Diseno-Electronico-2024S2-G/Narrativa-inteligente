import base64
import httpx
from langchain_core.prompts import ChatPromptTemplate, MessagesPlaceholder
from langchain_core.messages import HumanMessage
from langchain_openai import ChatOpenAI
from deep_translator import GoogleTranslator
from gtts import gTTS

def get_b64_image(image_path):
    with open(image_path, "rb") as image_file:
        return base64.b64encode(image_file.read()).decode("utf-8")

def text_to_voice(text, fout="output.mp3"):
    tts = gTTS(text=text, lang='es')
    tts.save(fout)

image_path = "/home/xhapa/Documents/PROGRAMMING/Projects/Narrativa-inteligente/python_backend/api/images/museos-mas-importantes-del-mundo.jpg"  # Update with your local image path
image_data = get_b64_image(image_path)

system_msg = 'You are a helpful assistant for blind people'

message = HumanMessage(
    content=[
        {
            "type": "text", 
            "text": "What is in the image?"
        },
        {
            "type": "image_url",
            "image_url": {"url": f"data:image/jpeg;base64,{image_data}"},
        },
    ]
)

prompt_template = ChatPromptTemplate([
    ("system", system_msg),
    MessagesPlaceholder("msgs")
])

llm = ChatOpenAI(model='llava', base_url="http://localhost:11434/v1", api_key="ollama")

chain = prompt_template | llm

ai_msg = chain.invoke({"msgs": [message]})
print(ai_msg.content)

translated = GoogleTranslator(source='auto', target='es').translate(ai_msg.content)
text_to_voice(translated)