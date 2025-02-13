import streamlit as st
import json
from deep_translator import GoogleTranslator

# Función para traducir el texto ingresado al inglés
def translate_to_english(text):
    text = str(text)  # Convertimos a string para evitar errores
    if text.strip():
        return GoogleTranslator(source='auto', target='en').translate(text)
    return text

# Configuración de la página
st.set_page_config(page_title="Formulario de Usuario", page_icon="📋")

st.title("📋 Formulario de Usuario")

# Sección de datos del usuario
st.sidebar.header("Información del Usuario")

user_data = {
    "user_name": st.sidebar.text_input("Nombre", "Juan"),
    "user_age": st.sidebar.number_input("Edad", min_value=1, max_value=100, value=30),
    "total_blind": st.sidebar.radio("¿Ceguera total?", ["Sí", "No"], index=0),
    "blind_details": st.sidebar.text_input("Si tiene visión parcial, ¿qué puede ver?", ""),
    "user_lang": st.sidebar.text_input("Idiomas", "Español, Inglés"),
    "braille": st.sidebar.radio("¿Usa Braille?", ["Sí", "No"], index=1),
    "devices_details": st.sidebar.text_area("Lector de pantalla u otras herramientas", ""),
    "mobility": st.sidebar.text_input("Ayuda para moverse", "Bastón blanco"),
    "independence_level": st.sidebar.selectbox("Nivel de independencia", ["Bajo", "Medio", "Alto"], index=2),
    "use_case": st.sidebar.text_area("¿En qué situaciones usará el dispositivo?", "Leer texto, reconocer objetos"),
    "reading_speed": st.sidebar.selectbox("Velocidad de lectura preferida", ["Lenta", "Normal", "Rápida"], index=1),
    "voice_preference": st.sidebar.selectbox("Preferencia de voz", ["Masculina", "Femenina", "Sin preferencia"], index=2),
    "tone_preference": st.sidebar.selectbox("Tono de voz", ["Formal", "Natural"], index=1),
    "detail_level": st.sidebar.selectbox("Nivel de detalle en la descripción", ["Breve", "Detallado"], index=1),
    "extra_info": st.sidebar.radio("¿Incluir información extra (colores, texturas)?", ["Sí", "No"], index=0),
    "current_devices": st.sidebar.text_area("Dispositivos que usa", "Teléfono inteligente, Computador con lector de pantalla"),
    "additional_details": st.sidebar.text_area("Información adicional", "")
}

# Traducción automática de cada entrada al inglés
translated_user_data = {key: translate_to_english(value) for key, value in user_data.items()}

st.sidebar.markdown("---")

# Guardar los datos en un archivo JSON
if st.sidebar.button("Guardar Datos"):
    with open("user_data.json", "w") as f:
        json.dump(translated_user_data, f, indent=4)
    st.sidebar.success("✅ ¡Datos guardados correctamente!")

# Mostrar datos capturados en español y su traducción en inglés
st.subheader("👤 Resumen de Datos del Usuario:")
st.write("📌 **Datos originales (Español):**")
st.json(user_data)

st.write("🌍 **Datos traducidos (Inglés):**")
st.json(translated_user_data)