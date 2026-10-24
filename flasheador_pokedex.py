import os
import shutil
import subprocess
import sys
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, scrolledtext, ttk
import webbrowser

# Blindaje contra fallos de PySerial al iniciar
try:
  import serial.tools.list_ports

  SERIAL_DISPONIBLE = True
except ImportError:
  SERIAL_DISPONIBLE = False


class PokedexApp:

  def __init__(self, root):
    self.root = root
    self.root.title("Panel de Control - Mini Pokedex ESP32")

    # Definir tamaño y centrar ventana principal automáticamente
    ancho, alto = 800, 680
    self.centrar_ventana(self.root, ancho, alto)
    self.root.minsize(700, 580)

    # Rutas por defecto basadas en la ubicación actual del script
    self.base_dir = os.path.dirname(os.path.abspath(__file__))
    default_ino = os.path.join(self.base_dir, "poke151.ino")
    default_header = os.path.join(self.base_dir, "pokemon_data.h")

    # Detección por defecto de la librería local en el proyecto
    default_lib = os.path.join(self.base_dir, "libraries", "TFT_eSPI")
    if not os.path.exists(default_lib):
      default_lib = os.path.join(self.base_dir, "TFT_eSPI")
      if not os.path.exists(default_lib):
        default_lib = os.path.join(self.base_dir, "libraries")

    # --- ENCABEZADO Y BOTÓN DE CRÉDITOS ---
    frame_superior = tk.Frame(root)
    frame_superior.pack(fill=tk.X, padx=15, pady=8)

    lbl_titulo = tk.Label(
        frame_superior,
        text=(
            "Gestor y Flasheador - Mini Pokedex (16MB Flash / Portátil local)"
        ),
        font=("Arial", 11, "bold"),
    )
    lbl_titulo.pack(side=tk.LEFT)

    btn_info = tk.Button(
        frame_superior,
        text="ℹ️ Legal y Créditos",
        command=self.mostrar_info,
        bg="#e0e0e0",
        font=("Arial", 9, "bold"),
        padx=5,
    )
    btn_info.pack(side=tk.RIGHT)

    # --- PANEL DE CONFIGURACIÓN DE RUTAS ---
    frame_rutas = tk.LabelFrame(
        root, text=" Configuración de Rutas y Archivos ", font=("Arial", 9, "bold")
    )
    frame_rutas.pack(fill=tk.X, padx=15, pady=5)

    # 1. Ruta del .ino / Proyecto
    row1 = tk.Frame(frame_rutas)
    row1.pack(fill=tk.X, padx=10, pady=4)
    tk.Label(row1, text="Archivo .ino:", width=14, anchor="w").pack(
        side=tk.LEFT
    )
    self.entry_ino = tk.Entry(row1, width=60)
    self.entry_ino.insert(0, default_ino)
    self.entry_ino.pack(side=tk.LEFT, padx=5, expand=True, fill=tk.X)
    tk.Button(row1, text="Examinar", command=self.buscar_ino).pack(side=tk.LEFT)

    # 2. Ruta de pokemon_data.h
    row2 = tk.Frame(frame_rutas)
    row2.pack(fill=tk.X, padx=10, pady=4)
    tk.Label(row2, text="pokemon_data.h:", width=14, anchor="w").pack(
        side=tk.LEFT
    )
    self.entry_header = tk.Entry(row2, width=60)
    self.entry_header.insert(0, default_header)
    self.entry_header.pack(side=tk.LEFT, padx=5, expand=True, fill=tk.X)
    tk.Button(row2, text="Examinar", command=self.buscar_header).pack(
        side=tk.LEFT
    )

    # 3. Ruta de la librería local (TFT_eSPI)
    row3 = tk.Frame(frame_rutas)
    row3.pack(fill=tk.X, padx=10, pady=4)
    tk.Label(row3, text="Carpeta Libraries:", width=14, anchor="w").pack(
        side=tk.LEFT
    )
    self.entry_lib = tk.Entry(row3, width=60)
    self.entry_lib.insert(0, default_lib)
    self.entry_lib.pack(side=tk.LEFT, padx=5, expand=True, fill=tk.X)
    tk.Button(row3, text="Examinar", command=self.buscar_lib).pack(side=tk.LEFT)

    # --- PANEL DE CONTROLES (PUERTO Y BOTÓN) ---
    frame_controles = tk.Frame(root)
    frame_controles.pack(pady=10, fill=tk.X, padx=15)

    tk.Label(frame_controles, text="Puerto COM:").pack(
        side=tk.LEFT, padx=(0, 5)
    )

    self.combo_puertos = ttk.Combobox(
        frame_controles, state="readonly", width=22
    )
    self.combo_puertos.pack(side=tk.LEFT, padx=5)

    btn_actualizar = tk.Button(
        frame_controles,
        text="🔄 Actualizar",
        command=self.cargar_puertos_hilo,
        padx=5,
    )
    btn_actualizar.pack(side=tk.LEFT, padx=5)

    self.btn_subir = tk.Button(
        frame_controles,
        text="🚀 Compilar y Subir",
        command=self.iniciar_proceso_hilo,
        bg="#4CAF50",
        fg="white",
        font=("Arial", 10, "bold"),
        padx=10,
    )
    self.btn_subir.pack(side=tk.RIGHT, padx=5)

    # --- CONSOLA EN TIEMPO REAL ---
    self.consola = scrolledtext.ScrolledText(
        root, wrap=tk.WORD, bg="#1e1e1e", fg="#00ff00", font=("Consolas", 10)
    )
    self.consola.pack(expand=True, fill=tk.BOTH, padx=15, pady=5)

    self.log(
        "Panel listo por Tipitochen. Rutas cargadas y listas para compilar.\n"
    )

    if not SERIAL_DISPONIBLE:
      self.log(
          "[!] Advertencia: La librería 'pyserial' no está instalada en este"
          " entorno."
      )
      self.log(
          "    Ejecuta en tu terminal: python -m pip install pyserial esptool"
      )

    # Cargar puertos al iniciar
    self.cargar_puertos()

  def centrar_ventana(self, ventana, ancho, alto):
    ventana.update_idletasks()
    screen_width = ventana.winfo_screenwidth()
    screen_height = ventana.winfo_screenheight()
    x = (screen_width - ancho) // 2
    y = (screen_height - alto) // 2
    ventana.geometry(f"{ancho}x{alto}+{x}+{y}")

  def log(self, mensaje):
    try:
      self.consola.insert(tk.END, mensaje + "\n")
      self.consola.see(tk.END)
    except Exception:
      pass

  def mostrar_info(self):
    ventana_info = tk.Toplevel(self.root)
    ventana_info.title("Acerca de / Legal - Mini Pokedex")

    self.centrar_ventana(ventana_info, 450, 320)
    ventana_info.resizable(False, False)
    ventana_info.grab_set()

    tk.Label(
        ventana_info,
        text="🎮 Mini Pokedex ESP32 (Open Source)",
        font=("Arial", 11, "bold"),
    ).pack(pady=(15, 5))

    texto_legal = (
        "• Creado con pasión por **Tipitochen** (De fans para fans).\n"
        "• Proyecto sin ningún tipo de fin comercial ni de lucro.\n"
        "• Datos obtenidos mediante PokéAPI.\n"
        "• Pokémon y sus nombres son marcas registradas de Nintendo,\n"
        "  GAME FREAK y The Pokémon Company.\n\n"
        "¡Suscríbete al canal de YouTube para más proyectos de hardware,\n"
        "consolaizaciones y retro gaming!"
    )

    lbl_desc = tk.Label(
        ventana_info, text=texto_legal, justify=tk.LEFT, font=("Arial", 9)
    )
    lbl_desc.pack(padx=20, pady=5)

    btn_youtube = tk.Button(
        ventana_info,
        text="📺 Visitar Canal de YouTube de Tipitochen",
        bg="#FF0000",
        fg="white",
        font=("Arial", 9, "bold"),
        command=lambda: webbrowser.open("https://www.youtube.com/@Tipitochen"),
    )
    btn_youtube.pack(pady=15)

    tk.Button(
        ventana_info,
        text="Cerrar",
        command=ventana_info.destroy,
        width=10,
    ).pack()

  def buscar_ino(self):
    archivo = filedialog.askopenfilename(
        title="Seleccionar archivo .ino", filetypes=[("Arduino Sketch", "*.ino")]
    )
    if archivo:
      self.entry_ino.delete(0, tk.END)
      self.entry_ino.insert(0, archivo)

  def buscar_header(self):
    archivo = filedialog.askopenfilename(
        title="Seleccionar pokemon_data.h", filetypes=[("Header Files", "*.h")]
    )
    if archivo:
      self.entry_header.delete(0, tk.END)
      self.entry_header.insert(0, archivo)

  def buscar_lib(self):
    carpeta = filedialog.askdirectory(title="Seleccionar carpeta de librerías")
    if carpeta:
      self.entry_lib.delete(0, tk.END)
      self.entry_lib.insert(0, carpeta)

  def cargar_puertos_hilo(self):
    # Actualización manual al presionar el botón
    self.cargar_puertos()
    self.log("[i] Lista de puertos actualizada.")

  def cargar_puertos(self):
    self.combo_puertos.set("")
    lista_opciones = []
    puerto_sugerido = None

    if not SERIAL_DISPONIBLE:
      self.log(
          "[!] PySerial no disponible. No se pueden buscar puertos serie."
      )
      return

    try:
      puertos = list(serial.tools.list_ports.comports())
    except Exception as e:
      self.log(f"[❌ Error al listar puertos]: {e}")
      puertos = []

    for p in puertos:
      texto = f"{p.device} - {p.description}"
      lista_opciones.append(texto)

      if any(
          chip in p.description.lower()
          for chip in [
              "cp210",
              "ch340",
              "usb-serial",
              "uart",
              "esp32",
              "silabs",
              "usb serial",
          ]
      ):
        puerto_sugerido = texto

    self.combo_puertos["values"] = lista_opciones

    if puerto_sugerido:
      self.combo_puertos.set(puerto_sugerido)
      self.log(f"[✓] ESP32 detectado: {puerto_sugerido}")
    elif lista_opciones:
      self.combo_puertos.current(0)
      self.log(f"[!] Puerto seleccionado: {lista_opciones[0]}")
    else:
      self.log(
          "[❌ Alerta] No hay puertos COM detectados. Conecta tu ESP32 por USB."
      )

  def iniciar_proceso_hilo(self):
    seleccion = self.combo_puertos.get()
    if not seleccion:
      messagebox.showwarning(
          "Aviso", "Selecciona un puerto COM antes de continuar."
      )
      return

    self.btn_subir.config(state=tk.DISABLED)
    threading.Thread(
        target=self.compilar_y_subir, args=(seleccion,), daemon=True
    ).start()

  def compilar_y_subir(self, seleccion_puerto):
    try:
      puerto = seleccion_puerto.split(" - ")[0]
      fqbn = "esp32:esp32:esp32"

      path_ino = self.entry_ino.get().strip()
      path_header = self.entry_header.get().strip()
      path_lib = self.entry_lib.get().strip()

      if not os.path.exists(path_ino):
        self.log(f"\n[❌ Error] No se encuentra el archivo .ino en:\n{path_ino}")
        self.btn_subir.config(state=tk.NORMAL)
        return

      ruta_sketch = os.path.dirname(path_ino)

      if path_header and os.path.exists(path_header):
        self.log(f"[i] Archivo de datos verificado: {path_header}")

      local_cli = os.path.join(ruta_sketch, "arduino-cli.exe")
      if os.path.exists(local_cli):
        arduino_cli_path = local_cli
      else:
        arduino_cli_path = shutil.which("arduino-cli")

      if not arduino_cli_path or not os.path.exists(arduino_cli_path):
        self.log(
            "\n[❌ Error] No se encontró el ejecutable de 'arduino-cli'."
        )
        messagebox.showerror(
            "Error", "No se encontró arduino-cli en el sistema ni en la carpeta."
        )
        self.btn_subir.config(state=tk.NORMAL)
        return

      args_compile = [
          "--board-options",
          "PartitionScheme=huge_app,FlashSize=16M",
      ]

      if path_lib and os.path.exists(path_lib):
        parent_lib_dir = (
            os.path.dirname(path_lib)
            if os.path.basename(path_lib).lower() == "tft_espi"
            else path_lib
        )
        args_compile.extend(["--libraries", parent_lib_dir])
        self.log(f"[i] Usando librería local desde: {path_lib}")

      self.log(
          "\n🚀 Compilando proyecto para ESP32 (16MB / Huge App / Librerías"
          " Locales)..."
      )

      cmd_compile = [
          arduino_cli_path,
          "compile",
          "--fqbn",
          fqbn,
      ] + args_compile + [ruta_sketch]

      proceso = subprocess.Popen(
          cmd_compile,
          stdout=subprocess.PIPE,
          stderr=subprocess.STDOUT,
          text=True,
          encoding="utf-8",
          errors="replace",
      )

      for linea in proceso.stdout:
        self.log(linea.strip())

      proceso.wait()

      if proceso.returncode != 0:
        self.log("\n[❌ ERROR]: La compilación falló.")
        messagebox.showerror("Error", "Ocurrió un error durante la compilación.")
        self.btn_subir.config(state=tk.NORMAL)
        return

      self.log("\n[✓] ¡Compilación finalizada con éxito!")
      self.log(f"📤 Subiendo firmware al ESP32 en el puerto {puerto}...")

      args_upload = [
          "--board-options",
          "PartitionScheme=huge_app,FlashSize=16M",
      ]

      cmd_upload = [
          arduino_cli_path,
          "upload",
          "-p",
          puerto,
          "--fqbn",
          fqbn,
      ] + args_upload + [ruta_sketch]

      proceso_subida = subprocess.Popen(
          cmd_upload,
          stdout=subprocess.PIPE,
          stderr=subprocess.STDOUT,
          text=True,
          encoding="utf-8",
          errors="replace",
      )

      for linea in proceso_subida.stdout:
        self.log(linea.strip())

      proceso_subida.wait()

      if proceso_subida.returncode != 0:
        self.log("\n[❌ ERROR]: Falló la subida al ESP32.")
        messagebox.showerror(
            "Error", "No se pudo subir el firmware al puerto COM."
        )
      else:
        self.log(
            "\n✨ ¡PROCESO COMPLETADO! Mini Pokedex actualizada con éxito."
        )
        messagebox.showinfo(
            "Éxito", "¡El firmware se ha subido correctamente al ESP32!"
        )

    except Exception as ex:
      self.log(f"\n[❌ Excepción crítica]: {ex}")
    finally:
      self.btn_subir.config(state=tk.NORMAL)


if __name__ == "__main__":
  root = tk.Tk()
  app = PokedexApp(root)
  root.mainloop()