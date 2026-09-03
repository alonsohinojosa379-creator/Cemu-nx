#include "localization.h"

#include <switch.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <utility>

namespace LauncherLocalization
{
namespace
{
std::string s_preference="system";
std::string s_language="en";
std::unordered_map<std::string,std::string> s_translations;
const std::vector<Language> s_languages={{"system","System"},{"en","English"},{"fr","Français"},{"de","Deutsch"},{"es","Español"},{"it","Italiano"},{"pt","Português"},{"zh-CN","简体中文"},{"zh-TW","繁體中文"}};

struct Entry { const char* en; const char* fr; const char* de; const char* es; const char* it; const char* pt; };
constexpr Entry ENTRIES[]={
 {"System","Système","System","Sistema","Sistema","Sistema"},
 {"Language","Langue","Sprache","Idioma","Lingua","Idioma"},
 {"Launcher","Lanceur","Launcher","Lanzador","Launcher","Launcher"},
 {"Settings","Paramètres","Einstellungen","Ajustes","Impostazioni","Definições"},
 {"Game settings","Paramètres du jeu","Spieleinstellungen","Ajustes del juego","Impostazioni gioco","Definições do jogo"},
 {"Library & storage","Bibliothèque et stockage","Bibliothek & Speicher","Biblioteca y almacenamiento","Libreria e archiviazione","Biblioteca e armazenamento"},
 {"Game folders","Dossiers de jeux","Spieleordner","Carpetas de juegos","Cartelle dei giochi","Pastas de jogos"},
 {"File manager","Gestionnaire de fichiers","Dateimanager","Gestor de archivos","Gestione file","Gestor de ficheiros"},
 {"SMB network shares","Partages réseau SMB","SMB-Netzwerkfreigaben","Recursos SMB","Condivisioni SMB","Partilhas SMB"},
 {"Download covers","Télécharger les jaquettes","Cover herunterladen","Descargar carátulas","Scarica copertine","Transferir capas"},
 {"Cover settings","Paramètres de la jaquette","Cover-Einstellungen","Ajustes de la carátula","Impostazioni copertina","Definições da capa"},
 {"Download from SteamGridDB","Télécharger depuis SteamGridDB","Von SteamGridDB herunterladen","Descargar desde SteamGridDB","Scarica da SteamGridDB","Transferir do SteamGridDB"},
 {"Import cover from file","Importer une jaquette depuis un fichier","Cover aus Datei importieren","Importar carátula desde un archivo","Importa copertina da file","Importar capa de um ficheiro"},
 {"Online artwork","Illustration en ligne","Online-Cover","Ilustración en línea","Immagine online","Imagem online"},
 {"Local image","Image locale","Lokales Bild","Imagen local","Immagine locale","Imagem local"},
 {"Search SteamGridDB and replace this game's custom cover with selected online artwork.","Recherche sur SteamGridDB et remplace la jaquette personnalisée de ce jeu par l'illustration sélectionnée.","Durchsucht SteamGridDB und ersetzt das benutzerdefinierte Cover dieses Spiels durch das ausgewählte Online-Bild.","Busca en SteamGridDB y sustituye la carátula personalizada de este juego por la ilustración seleccionada.","Cerca su SteamGridDB e sostituisce la copertina personalizzata del gioco con l'immagine selezionata.","Pesquisa no SteamGridDB e substitui a capa personalizada deste jogo pela imagem selecionada."},
 {"Choose a PNG, JPEG, WebP or BMP image from SD, USB or SMB storage. It is validated and saved safely as PNG.","Choisissez une image PNG, JPEG, WebP ou BMP sur un stockage SD, USB ou SMB. Elle est vérifiée et enregistrée en toute sécurité au format PNG.","Wähle ein PNG-, JPEG-, WebP- oder BMP-Bild von SD-, USB- oder SMB-Speicher. Es wird geprüft und sicher als PNG gespeichert.","Elige una imagen PNG, JPEG, WebP o BMP del almacenamiento SD, USB o SMB. Se valida y guarda de forma segura como PNG.","Scegli un'immagine PNG, JPEG, WebP o BMP da una memoria SD, USB o SMB. Verrà verificata e salvata in modo sicuro come PNG.","Escolha uma imagem PNG, JPEG, WebP ou BMP do armazenamento SD, USB ou SMB. É validada e guardada em segurança como PNG."},
 {"Remove custom cover","Supprimer la jaquette personnalisée","Benutzerdefiniertes Cover entfernen","Quitar carátula personalizada","Rimuovi copertina personalizzata","Remover capa personalizada"},
 {"Select local cover","Sélectionner une jaquette locale","Lokales Cover auswählen","Seleccionar carátula local","Seleziona copertina locale","Selecionar capa local"},
 {"Cover imported","Jaquette importée","Cover importiert","Carátula importada","Copertina importata","Capa importada"},
 {"Custom cover removed","Jaquette personnalisée supprimée","Benutzerdefiniertes Cover entfernt","Carátula personalizada eliminada","Copertina personalizzata rimossa","Capa personalizada removida"},
 {"Cover import failed","Échec de l'importation de la jaquette","Cover-Import fehlgeschlagen","Error al importar la carátula","Importazione copertina non riuscita","Falha ao importar a capa"},
 {"Cover removal failed","Échec de la suppression de la jaquette","Cover konnte nicht entfernt werden","Error al eliminar la carátula","Rimozione copertina non riuscita","Falha ao remover a capa"},
 {"Remove custom cover?","Supprimer la jaquette personnalisée ?","Benutzerdefiniertes Cover entfernen?","¿Quitar la carátula personalizada?","Rimuovere la copertina personalizzata?","Remover a capa personalizada?"},
 {"The downloaded or imported cover will be deleted.","La jaquette téléchargée ou importée sera supprimée.","Das heruntergeladene oder importierte Cover wird gelöscht.","Se eliminará la carátula descargada o importada.","La copertina scaricata o importata verrà eliminata.","A capa transferida ou importada será eliminada."},
 {"The launcher will use the game's embedded artwork when available.","Le lanceur utilisera l'illustration intégrée au jeu lorsqu'elle est disponible.","Der Launcher verwendet das im Spiel enthaltene Bild, sofern verfügbar.","El lanzador usará la ilustración integrada del juego cuando esté disponible.","Il launcher userà l'immagine integrata nel gioco quando disponibile.","O iniciador utilizará a imagem integrada do jogo quando disponível."},
 {"Search","Rechercher","Suchen","Buscar","Cerca","Pesquisar"},
 {"Favorites","Favoris","Favoriten","Favoritos","Preferiti","Favoritos"},
 {"Collections","Collections","Sammlungen","Colecciones","Raccolte","Coleções"},
 {"All games","Tous les jeux","Alle Spiele","Todos los juegos","Tutti i giochi","Todos os jogos"},
 {"Reset","Réinitialiser","Zurücksetzen","Restablecer","Ripristina","Repor"},
 {"Setting reset to default","Paramètre réinitialisé","Einstellung zurückgesetzt","Ajuste restablecido","Impostazione ripristinata","Definição reposta"},
 {"Check for Updates","Rechercher des mises à jour","Nach Updates suchen","Buscar actualizaciones","Controlla aggiornamenti","Procurar atualizações"},
 {"SteamGridDB API key","Clé API SteamGridDB","SteamGridDB-API-Schlüssel","Clave API de SteamGridDB","Chiave API SteamGridDB","Chave API SteamGridDB"},
 {"Loading game library...","Chargement de la bibliothèque...","Spielebibliothek wird geladen...","Cargando la biblioteca...","Caricamento della libreria...","A carregar a biblioteca..."},
 {"The first page will appear as soon as it is ready.","La première page s'affichera dès qu'elle sera prête.","Die erste Seite erscheint, sobald sie bereit ist.","La primera página aparecerá cuando esté lista.","La prima pagina apparirà appena pronta.","A primeira página aparecerá quando estiver pronta."},
 {"Exit Cemu?","Quitter Cemu ?","Cemu beenden?","¿Salir de Cemu?","Uscire da Cemu?","Sair do Cemu?"},
 {"Return to the HOME Menu?","Retourner au menu HOME ?","Zum HOME-Menü zurückkehren?","¿Volver al menú HOME?","Tornare al menu HOME?","Voltar ao Menu HOME?"},
 {"Closing Cemu...","Fermeture de Cemu...","Cemu wird beendet...","Cerrando Cemu...","Chiusura di Cemu...","A fechar o Cemu..."},
 {"Install","Installer","Installieren","Instalar","Installa","Instalar"},
 {"Applet mode installer","Installation en mode applet","Applet-Modus-Installer","Instalador en modo applet","Installazione in modalità applet","Instalador em modo applet"},
 {"Cemu is running in applet mode.","Cemu fonctionne en mode applet.","Cemu läuft im Applet-Modus.","Cemu se ejecuta en modo applet.","Cemu è in modalità applet.","O Cemu está em modo applet."},
 {"Install a HOME Menu shortcut to run Cemu with full memory and normal performance.","Installez un raccourci HOME pour utiliser toute la mémoire.","Installiere eine HOME-Verknüpfung für vollen Speicher.","Instala un acceso HOME para usar toda la memoria.","Installa un collegamento HOME per usare tutta la memoria.","Instale um atalho HOME para usar toda a memória."},
 {"Back","Retour","Zurück","Atrás","Indietro","Voltar"},
 {"Choose","Choisir","Auswählen","Elegir","Scegli","Escolher"},
 {"Info","Info","Info","Info","Info","Info"},
 {"On","Activé","Ein","Sí","Attivo","Ligado"},
 {"Off","Désactivé","Aus","No","Disattivato","Desligado"},
 {"Enabled","Activé","Aktiviert","Activado","Attivato","Ativado"},
 {"Disabled","Désactivé","Deaktiviert","Desactivado","Disattivato","Desativado"},
 {"Change","Modifier","Ändern","Cambiar","Modifica","Alterar"},
 {"Select","Sélectionner","Auswählen","Seleccionar","Seleziona","Selecionar"},
 {"Open","Ouvrir","Öffnen","Abrir","Apri","Abrir"},
 {"Close","Fermer","Schließen","Cerrar","Chiudi","Fechar"},
 {"Cancel","Annuler","Abbrechen","Cancelar","Annulla","Cancelar"},
 {"Continue","Continuer","Weiter","Continuar","Continua","Continuar"},
 {"Assign","Assigner","Zuweisen","Asignar","Assegna","Atribuir"},
 {"Use artwork","Utiliser l'image","Bild verwenden","Usar imagen","Usa immagine","Usar imagem"},
 {"Edit / choose","Modifier / choisir","Bearbeiten / wählen","Editar / elegir","Modifica / scegli","Editar / escolher"},
 {"Details / delete","Détails / supprimer","Details / löschen","Detalles / eliminar","Dettagli / elimina","Detalhes / eliminar"},
 {"Hold to clear","Maintenir pour effacer","Zum Löschen halten","Mantener para borrar","Tieni premuto per cancellare","Manter para limpar"},
 {"Hold to cancel","Maintenir pour annuler","Zum Abbrechen halten","Mantener para cancelar","Tieni premuto per annullare","Manter para cancelar"},
 {"Launch","Lancer","Starten","Iniciar","Avvia","Iniciar"},
 {"Sort","Trier","Sortieren","Ordenar","Ordina","Ordenar"},
 {"Game Menu","Menu du jeu","Spielmenü","Menú del juego","Menu gioco","Menu do jogo"},
 {"Filter","Filtrer","Filtern","Filtrar","Filtra","Filtrar"},
 {"Page","Page","Seite","Página","Pagina","Página"},
 {"Quit","Quitter","Beenden","Salir","Esci","Sair"},
 {"CPU / Emulation","CPU / Émulation","CPU / Emulation","CPU / Emulación","CPU / Emulazione","CPU / Emulação"},
 {"Graphics","Graphismes","Grafik","Gráficos","Grafica","Gráficos"},
 {"Frame Generation","Génération d'images","Frame-Generierung","Generación de fotogramas","Generazione fotogrammi","Geração de fotogramas"},
 {"Audio","Audio","Audio","Audio","Audio","Áudio"},
 {"Overlay","Surimpression","Overlay","Superposición","Sovrimpressione","Sobreposição"},
 {"Controller / Input","Manette / Entrées","Controller / Eingabe","Mando / Entrada","Controller / Input","Comando / Entrada"},
 {"USB Accessories","Accessoires USB","USB-Zubehör","Accesorios USB","Accessori USB","Acessórios USB"},
 {"CPU mode","Mode CPU","CPU-Modus","Modo de CPU","Modalità CPU","Modo de CPU"},
 {"CPU timer speed","Vitesse du minuteur CPU","CPU-Timer-Geschwindigkeit","Velocidad del temporizador CPU","Velocità timer CPU","Velocidade do temporizador CPU"},
 {"Hardware video decoding","Décodage vidéo matériel","Hardware-Videodekodierung","Decodificación de vídeo por hardware","Decodifica video hardware","Descodificação de vídeo por hardware"},
 {"Renderer","Moteur de rendu","Renderer","Renderizador","Renderer","Renderizador"},
 {"Vulkan (NVK)","Vulkan (NVK)","Vulkan (NVK)","Vulkan (NVK)","Vulkan (NVK)","Vulkan (NVK)"},
 {"OpenGL (NVC0)","OpenGL (NVC0)","OpenGL (NVC0)","OpenGL (NVC0)","OpenGL (NVC0)","OpenGL (NVC0)"},
 {"OpenGL (Zink/NVK)","OpenGL (Zink/NVK)","OpenGL (Zink/NVK)","OpenGL (Zink/NVK)","OpenGL (Zink/NVK)","OpenGL (Zink/NVK)"},
 {"Triple buffering","Triple mise en mémoire tampon","Dreifachpufferung","Triple búfer","Triplo buffering","Buffer triplo"},
 {"Async shader compile","Compilation asynchrone des shaders","Asynchrone Shader-Kompilierung","Compilación asíncrona de shaders","Compilazione shader asincrona","Compilação assíncrona de shaders"},
 {"Accurate barriers (Vulkan)","Barrières précises (Vulkan)","Präzise Barrieren (Vulkan)","Barreras precisas (Vulkan)","Barriere accurate (Vulkan)","Barreiras precisas (Vulkan)"},
 {"Upscale filter","Filtre d'agrandissement","Hochskalierungsfilter","Filtro de ampliación","Filtro di ingrandimento","Filtro de ampliação"},
 {"Downscale filter","Filtre de réduction","Herunterskalierungsfilter","Filtro de reducción","Filtro di riduzione","Filtro de redução"},
 {"Fullscreen scaling","Mise à l'échelle plein écran","Vollbildskalierung","Escalado de pantalla completa","Ridimensionamento a schermo intero","Escala de ecrã inteiro"},
 {"GamePad screen","Écran du GamePad","GamePad-Bildschirm","Pantalla del GamePad","Schermo GamePad","Ecrã do GamePad"},
 {"TV volume","Volume TV","TV-Lautstärke","Volumen de TV","Volume TV","Volume da TV"},
 {"GamePad audio","Audio du GamePad","GamePad-Audio","Audio del GamePad","Audio GamePad","Áudio do GamePad"},
 {"GamePad volume","Volume du GamePad","GamePad-Lautstärke","Volumen del GamePad","Volume GamePad","Volume do GamePad"},
 {"Audio latency","Latence audio","Audio-Latenz","Latencia de audio","Latenza audio","Latência de áudio"},
 {"Flow resolution","Résolution du flux","Flow-Auflösung","Resolución de flujo","Risoluzione del flusso","Resolução do fluxo"},
 {"Performance mode","Mode performance","Leistungsmodus","Modo rendimiento","Modalità prestazioni","Modo de desempenho"},
 {"FPS counter","Compteur de FPS","FPS-Zähler","Contador de FPS","Contatore FPS","Contador de FPS"},
 {"Overlay position","Position de la surimpression","Overlay-Position","Posición de la superposición","Posizione sovrimpressione","Posição da sobreposição"},
 {"Shader-compile notice","Notification de compilation des shaders","Shader-Kompilierungshinweis","Aviso de compilación de shaders","Avviso compilazione shader","Aviso de compilação de shaders"},
 {"Console language","Langue de la console","Konsolensprache","Idioma de la consola","Lingua console","Idioma da consola"},
 {"Controller type","Type de manette","Controller-Typ","Tipo de mando","Tipo controller","Tipo de comando"},
 {"Active players","Joueurs actifs","Aktive Spieler","Jugadores activos","Giocatori attivi","Jogadores ativos"},
 {"Rumble","Vibrations","Vibration","Vibración","Vibrazione","Vibração"},
 {"Stick dead zone","Zone morte du stick","Stick-Totzone","Zona muerta del stick","Zona morta stick","Zona morta do analógico"},
 {"Control mapping","Mappage des commandes","Tastenbelegung","Asignación de controles","Mappatura comandi","Mapeamento de controlos"},
 {"Skylanders Portal","Portail Skylanders","Skylanders-Portal","Portal de Skylanders","Portale Skylanders","Portal Skylanders"},
 {"Disney Infinity Base","Base Disney Infinity","Disney-Infinity-Basis","Base de Disney Infinity","Base Disney Infinity","Base Disney Infinity"},
 {"LEGO Dimensions Toypad","Toy Pad LEGO Dimensions","LEGO-Dimensions-Toypad","Toy Pad de LEGO Dimensions","Toy Pad LEGO Dimensions","Toy Pad LEGO Dimensions"},
 {"Theme","Thème","Design","Tema","Tema","Tema"},
 {"Games per row","Jeux par ligne","Spiele pro Reihe","Juegos por fila","Giochi per riga","Jogos por linha"},
 {"Rows per page","Lignes par page","Reihen pro Seite","Filas por página","Righe per pagina","Linhas por página"},
 {"Show game titles","Afficher les titres des jeux","Spieltitel anzeigen","Mostrar títulos de juegos","Mostra titoli dei giochi","Mostrar títulos dos jogos"},
 {"Show region flags","Afficher les drapeaux de région","Regionsflaggen anzeigen","Mostrar banderas de región","Mostra bandiere regionali","Mostrar bandeiras de região"},
 {"Show custom settings badges","Afficher les indicateurs de paramètres personnalisés","Markierungen für benutzerdefinierte Einstellungen anzeigen","Mostrar indicadores de ajustes personalizados","Mostra indicatori delle impostazioni personalizzate","Mostrar indicadores de definições personalizadas"},
 {"UI animations","Animations de l'interface","UI-Animationen","Animaciones de interfaz","Animazioni interfaccia","Animações da interface"},
 {"Sound effects","Effets sonores","Soundeffekte","Efectos de sonido","Effetti sonori","Efeitos sonoros"},
 {"Check updates at boot","Rechercher les mises à jour au démarrage","Beim Start nach Updates suchen","Buscar actualizaciones al iniciar","Controlla aggiornamenti all'avvio","Procurar atualizações ao iniciar"},
 {"Online & accounts","En ligne et comptes","Online & Konten","En línea y cuentas","Online e account","Online e contas"},
 {"Graphics packs","Packs graphiques","Grafikpakete","Paquetes gráficos","Pacchetti grafici","Pacotes gráficos"},
 {"Installed content","Contenu installé","Installierte Inhalte","Contenido instalado","Contenuti installati","Conteúdo instalado"},
 {"Install update / DLC / game","Installer une mise à jour / DLC / jeu","Update / DLC / Spiel installieren","Instalar actualización / DLC / juego","Installa aggiornamento / DLC / gioco","Instalar atualização / DLC / jogo"},
 {"missing only","manquantes uniquement","nur fehlende","solo faltantes","solo mancanti","apenas em falta"},
 {"New collection...","Nouvelle collection...","Neue Sammlung...","Nueva colección...","Nuova raccolta...","Nova coleção..."},
 {"Manage collections","Gérer les collections","Sammlungen verwalten","Gestionar colecciones","Gestisci raccolte","Gerir coleções"},
 {"Search...","Rechercher...","Suchen...","Buscar...","Cerca...","Pesquisar..."},
 {"Favorite","Favori","Favorit","Favorito","Preferito","Favorito"},
 {"Rename","Renommer","Umbenennen","Renombrar","Rinomina","Mudar nome"},
 {"Copy","Copier","Kopieren","Copiar","Copia","Copiar"},
 {"Move","Déplacer","Verschieben","Mover","Sposta","Mover"},
 {"Delete collection","Supprimer la collection","Sammlung löschen","Eliminar colección","Elimina raccolta","Eliminar coleção"},
 {"Filter library","Filtrer la bibliothèque","Bibliothek filtern","Filtrar biblioteca","Filtra libreria","Filtrar biblioteca"},
 {"Favorites & collections","Favoris et collections","Favoriten & Sammlungen","Favoritos y colecciones","Preferiti e raccolte","Favoritos e coleções"},
 {"Installing...","Installation...","Installation...","Instalando...","Installazione...","A instalar..."},
 {"HOME Menu shortcut installed.","Raccourci du menu HOME installé.","HOME-Menü-Verknüpfung installiert.","Acceso del menú HOME instalado.","Collegamento del menu HOME installato.","Atalho do Menu HOME instalado."},
 {"Active scans and network operations will be cancelled safely.","Les analyses et opérations réseau actives seront annulées en toute sécurité.","Aktive Scans und Netzwerkvorgänge werden sicher abgebrochen.","Los análisis y las operaciones de red se cancelarán de forma segura.","Le scansioni e le operazioni di rete attive verranno annullate in sicurezza.","Os varrimentos e as operações de rede ativos serão cancelados em segurança."},
 {"Finishing background operations safely.","Finalisation sécurisée des opérations en arrière-plan.","Hintergrundvorgänge werden sicher beendet.","Finalizando de forma segura las operaciones en segundo plano.","Chiusura sicura delle operazioni in background.","A concluir em segurança as operações em segundo plano."},
 {"CPU emulation","Émulation du CPU","CPU-Emulation","Emulación de CPU","Emulazione CPU","Emulação de CPU"},
 {"Game timing / compatibility","Synchronisation du jeu / compatibilité","Spiel-Timing / Kompatibilität","Temporización / compatibilidad","Temporizzazione / compatibilità","Temporização / compatibilidade"},
 {"Video playback","Lecture vidéo","Videowiedergabe","Reproducción de vídeo","Riproduzione video","Reprodução de vídeo"},
 {"Display backend","Moteur d'affichage","Grafik-Backend","Motor gráfico","Backend grafico","Backend gráfico"},
 {"Presentation","Présentation","Darstellung","Presentación","Presentazione","Apresentação"},
 {"Presentation / performance","Présentation / performances","Darstellung / Leistung","Presentación / rendimiento","Presentazione / prestazioni","Apresentação / desempenho"},
 {"Shader compilation","Compilation des shaders","Shader-Kompilierung","Compilación de shaders","Compilazione shader","Compilação de shaders"},
 {"Graphics compatibility","Compatibilité graphique","Grafikkompatibilität","Compatibilidad gráfica","Compatibilità grafica","Compatibilidade gráfica"},
 {"Image scaling","Mise à l'échelle de l'image","Bildskalierung","Escalado de imagen","Ridimensionamento immagine","Escala da imagem"},
 {"Display aspect ratio","Format d'affichage","Bildseitenverhältnis","Relación de aspecto","Proporzioni schermo","Proporção do ecrã"},
 {"Wii U GamePad display","Affichage du Wii U GamePad","Wii-U-GamePad-Anzeige","Pantalla del Wii U GamePad","Schermo Wii U GamePad","Ecrã do Wii U GamePad"},
 {"Audio output","Sortie audio","Audioausgabe","Salida de audio","Uscita audio","Saída de áudio"},
 {"Wii U GamePad audio","Audio du Wii U GamePad","Wii-U-GamePad-Audio","Audio del Wii U GamePad","Audio Wii U GamePad","Áudio do Wii U GamePad"},
 {"Audio latency / stability","Latence / stabilité audio","Audio-Latenz / Stabilität","Latencia / estabilidad de audio","Latenza / stabilità audio","Latência / estabilidade de áudio"},
 {"Frame generation","Génération d'images","Frame-Generierung","Generación de fotogramas","Generazione fotogrammi","Geração de fotogramas"},
 {"Frame generation quality","Qualité de génération d'images","Frame-Generierungsqualität","Calidad de generación de fotogramas","Qualità generazione fotogrammi","Qualidade da geração de fotogramas"},
 {"Frame generation performance","Performances de génération d'images","Frame-Generierungsleistung","Rendimiento de generación de fotogramas","Prestazioni generazione fotogrammi","Desempenho da geração de fotogramas"},
 {"Performance display","Affichage des performances","Leistungsanzeige","Indicador de rendimiento","Indicatore prestazioni","Indicador de desempenho"},
 {"Emulated controller","Manette émulée","Emulierter Controller","Mando emulado","Controller emulato","Comando emulado"},
 {"Controller input","Entrées de la manette","Controller-Eingabe","Entrada del mando","Input controller","Entrada do comando"},
 {"Controller feedback","Retour de la manette","Controller-Feedback","Respuesta del mando","Feedback controller","Resposta do comando"},
 {"Analog input","Entrée analogique","Analogeingabe","Entrada analógica","Input analogico","Entrada analógica"},
 {"Emulated USB accessory","Accessoire USB émulé","Emuliertes USB-Zubehör","Accesorio USB emulado","Accessorio USB emulato","Acessório USB emulado"},
 {"Wii U system language","Langue système Wii U","Wii-U-Systemsprache","Idioma del sistema Wii U","Lingua di sistema Wii U","Idioma do sistema Wii U"},
 {"Launcher appearance","Apparence du lanceur","Launcher-Darstellung","Apariencia del lanzador","Aspetto launcher","Aspeto do launcher"},
 {"Launcher language","Langue du lanceur","Launcher-Sprache","Idioma del lanzador","Lingua del launcher","Idioma do launcher"},
 {"Library layout","Disposition de la bibliothèque","Bibliothekslayout","Diseño de la biblioteca","Layout libreria","Disposição da biblioteca"},
 {"Launcher audio","Audio du lanceur","Launcher-Audio","Audio del lanzador","Audio launcher","Áudio do launcher"},
 {"Launcher updates","Mises à jour du lanceur","Launcher-Updates","Actualizaciones del lanzador","Aggiornamenti launcher","Atualizações do launcher"},
 {"Controller mapping","Mappage de la manette","Controller-Belegung","Asignación del mando","Mappatura controller","Mapeamento do comando"},
 {"Required component","Composant requis","Erforderliche Komponente","Componente requerido","Componente richiesto","Componente necessário"},
 {"Setting","Paramètre","Einstellung","Ajuste","Impostazione","Definição"},
 {"Per-game setting","Paramètre par jeu","Spielspezifische Einstellung","Ajuste por juego","Impostazione per gioco","Definição por jogo"},
 {"Global setting","Paramètre global","Globale Einstellung","Ajuste global","Impostazione globale","Definição global"},
 {"Launcher setting","Paramètre du lanceur","Launcher-Einstellung","Ajuste del lanzador","Impostazione launcher","Definição do launcher"},
 {"Launcher action","Action du lanceur","Launcher-Aktion","Acción del lanzador","Azione launcher","Ação do launcher"},
 {"Settings category","Catégorie de paramètres","Einstellungskategorie","Categoría de ajustes","Categoria impostazioni","Categoria de definições"},
 {"Per-game overrides","Remplacements par jeu","Spielspezifische Überschreibungen","Ajustes por juego","Override per gioco","Substituições por jogo"},
 {"Selects how Cemu executes the Wii U CPU. The multi-core recompiler is fastest on Switch. Use single-core only for compatibility testing; the interpreter is extremely slow and intended for debugging.",
  "Détermine comment Cemu exécute le CPU Wii U. Le recompilateur multicœur est le plus rapide sur Switch. N'utilisez le mode monocœur que pour tester la compatibilité ; l'interpréteur, extrêmement lent, sert au débogage.",
  "Legt fest, wie Cemu die Wii-U-CPU ausführt. Der Mehrkern-Recompiler ist auf Switch am schnellsten. Einkern nur für Kompatibilitätstests verwenden; der extrem langsame Interpreter ist zum Debuggen gedacht.",
  "Define cómo ejecuta Cemu la CPU de Wii U. El recompilador multinúcleo es el más rápido en Switch. Usa un solo núcleo únicamente para probar compatibilidad; el intérprete es muy lento y sirve para depuración.",
  "Sceglie come Cemu esegue la CPU Wii U. Il ricompilatore multicore è il più veloce su Switch. Usa il single-core solo per test di compatibilità; l'interprete è molto lento e serve al debug.",
  "Define como o Cemu executa o CPU da Wii U. O recompilador multinúcleo é o mais rápido na Switch. Use um núcleo só para testar compatibilidade; o interpretador é muito lento e destina-se à depuração."},
 {"Changes the speed of the emulated CPU timer without increasing the Switch CPU clock. Some game-specific fixes need a different timer rate, but the normal 1x value is the safest default.",
  "Modifie la vitesse du minuteur CPU émulé sans augmenter la fréquence du CPU de la Switch. Certains correctifs exigent une valeur différente, mais 1x reste le réglage par défaut le plus sûr.",
  "Ändert den emulierten CPU-Timer ohne den Switch-CPU-Takt zu erhöhen. Manche Spielekorrekturen benötigen einen anderen Wert; 1x ist der sicherste Standard.",
  "Cambia la velocidad del temporizador de CPU emulado sin aumentar la frecuencia de la CPU de Switch. Algunos juegos requieren otro valor, pero 1x es el ajuste más seguro.",
  "Cambia la velocità del timer CPU emulato senza aumentare il clock della CPU Switch. Alcuni giochi richiedono un valore diverso, ma 1x è l'impostazione più sicura.",
  "Altera a velocidade do temporizador do CPU emulado sem aumentar a frequência do CPU da Switch. Alguns jogos precisam de outro valor, mas 1x é a predefinição mais segura."},
 {"Uses the Switch hardware video decoder for Wii U H.264 movies. Disable it only when troubleshooting broken or missing in-game video playback.",
  "Utilise le décodeur vidéo matériel de la Switch pour les vidéos H.264 Wii U. Désactivez-le uniquement pour diagnostiquer des vidéos absentes ou défectueuses.",
  "Nutzt den Hardware-Videodecoder der Switch für Wii-U-H.264-Videos. Nur zur Fehlersuche bei fehlender oder fehlerhafter Wiedergabe deaktivieren.",
  "Usa el decodificador de vídeo por hardware de Switch para vídeos H.264 de Wii U. Desactívalo solo para diagnosticar vídeos ausentes o defectuosos.",
  "Usa il decoder video hardware di Switch per i filmati H.264 Wii U. Disattivalo solo per diagnosticare video mancanti o difettosi.",
  "Usa o descodificador de vídeo por hardware da Switch para vídeos H.264 da Wii U. Desative apenas para diagnosticar vídeos em falta ou com falhas."},
 {"Chooses the graphics backend in the unified Cemu core. Vulkan (NVK) is recommended and is required by LSFG. OpenGL uses native NVC0, while Zink runs Cemu's OpenGL renderer on NVK as an additional compatibility path.",
  "Choisit le moteur graphique du cœur Cemu unifié. Vulkan (NVK) est recommandé et requis par LSFG. OpenGL utilise NVC0 en natif, tandis que Zink exécute le moteur OpenGL de Cemu sur NVK comme solution de compatibilité supplémentaire.",
  "Wählt das Grafik-Backend des vereinheitlichten Cemu-Kerns. Vulkan (NVK) wird empfohlen und ist für LSFG erforderlich. OpenGL nutzt NVC0 nativ; Zink führt Cemus OpenGL-Renderer über NVK als zusätzlichen Kompatibilitätsweg aus.",
  "Elige el motor gráfico del núcleo Cemu unificado. Se recomienda Vulkan (NVK), que es obligatorio para LSFG. OpenGL usa NVC0 de forma nativa, mientras Zink ejecuta el renderizador OpenGL de Cemu sobre NVK como vía adicional de compatibilidad.",
  "Sceglie il backend grafico del core Cemu unificato. Vulkan (NVK) è consigliato ed è richiesto da LSFG. OpenGL usa NVC0 nativo, mentre Zink esegue il renderer OpenGL di Cemu su NVK come percorso di compatibilità aggiuntivo.",
  "Escolhe o backend gráfico do núcleo Cemu unificado. Vulkan (NVK) é recomendado e obrigatório para LSFG. OpenGL usa NVC0 nativo, enquanto o Zink executa o renderizador OpenGL do Cemu sobre NVK como caminho de compatibilidade adicional."},
 {"Synchronizes completed frames to the display refresh to reduce tearing. It can add latency or expose performance drops when a game cannot maintain its target frame rate.",
  "Synchronise les images avec le rafraîchissement de l'écran pour réduire les déchirures. Cela peut ajouter de la latence ou rendre les baisses de performances plus visibles.",
  "Synchronisiert fertige Bilder mit der Bildschirmaktualisierung, um Tearing zu verringern. Dies kann Latenz hinzufügen oder Leistungseinbrüche sichtbarer machen.",
  "Sincroniza los fotogramas con la actualización de pantalla para reducir el tearing. Puede añadir latencia o hacer más visibles las caídas de rendimiento.",
  "Sincronizza i fotogrammi con il refresh dello schermo per ridurre il tearing. Può aggiungere latenza o rendere più evidenti i cali di prestazioni.",
  "Sincroniza os fotogramas com a atualização do ecrã para reduzir cortes. Pode aumentar a latência ou tornar mais visíveis as quebras de desempenho."},
 {"Keeps a third Vulkan swapchain image available while another frame is being displayed. This can make presentation steadier under load, at the cost of some memory and potentially more latency.",
  "Conserve une troisième image Vulkan pendant l'affichage d'une autre. La présentation peut être plus stable en charge, au prix de mémoire et parfois de latence supplémentaires.",
  "Hält ein drittes Vulkan-Swapchain-Bild bereit. Das kann die Darstellung unter Last stabilisieren, kostet aber Speicher und möglicherweise zusätzliche Latenz.",
  "Mantiene disponible una tercera imagen de Vulkan. Puede estabilizar la presentación bajo carga, a costa de memoria y posiblemente más latencia.",
  "Mantiene disponibile una terza immagine Vulkan. Può stabilizzare la presentazione sotto carico, consumando memoria e forse aggiungendo latenza.",
  "Mantém uma terceira imagem Vulkan disponível. Pode estabilizar a apresentação sob carga, usando mais memória e possivelmente mais latência."},
 {"Compiles Vulkan shaders asynchronously to reduce long gameplay stalls. Newly encountered effects may be missing briefly while their shaders finish compiling.",
  "Compile les shaders Vulkan de façon asynchrone pour réduire les longs blocages. Les nouveaux effets peuvent manquer brièvement pendant leur compilation.",
  "Kompiliert Vulkan-Shader asynchron, um lange Hänger zu reduzieren. Neue Effekte können kurz fehlen, bis ihre Shader fertig sind.",
  "Compila shaders de Vulkan de forma asíncrona para reducir pausas largas. Los efectos nuevos pueden faltar brevemente mientras se compilan.",
  "Compila gli shader Vulkan in modo asincrono per ridurre i blocchi lunghi. I nuovi effetti possono mancare brevemente durante la compilazione.",
  "Compila shaders Vulkan de forma assíncrona para reduzir pausas longas. Efeitos novos podem faltar por instantes durante a compilação."},
 {"Uses more accurate Vulkan synchronization between rendering operations. Keep it enabled for correct effects; disabling it may improve performance slightly but can cause game-specific rendering errors.",
  "Utilise une synchronisation Vulkan plus précise entre les opérations. Gardez-la activée pour des effets corrects ; la désactiver peut légèrement accélérer mais provoquer des erreurs graphiques.",
  "Verwendet genauere Vulkan-Synchronisierung. Für korrekte Effekte aktiviert lassen; Deaktivieren kann etwas Leistung bringen, aber Grafikfehler verursachen.",
  "Usa una sincronización de Vulkan más precisa. Déjala activada para efectos correctos; desactivarla puede mejorar algo el rendimiento, pero causar errores gráficos.",
  "Usa una sincronizzazione Vulkan più accurata. Lasciala attiva per effetti corretti; disattivarla può migliorare leggermente le prestazioni ma causare errori grafici.",
  "Usa sincronização Vulkan mais precisa. Mantenha ativada para efeitos corretos; desativar pode melhorar ligeiramente o desempenho, mas causar erros gráficos."},
 {"Selects the filter used when the Wii U image is enlarged to the Switch display. Nearest is sharp and pixelated, while the other filters trade sharpness for smoother scaling.",
  "Choisit le filtre utilisé pour agrandir l'image Wii U. Le plus proche est net et pixellisé ; les autres lissent davantage au prix de netteté.",
  "Wählt den Filter zum Vergrößern des Wii-U-Bilds. Nearest ist scharf und pixelig; andere Filter skalieren glatter, aber weicher.",
  "Elige el filtro al ampliar la imagen de Wii U. Vecino más cercano es nítido y pixelado; los demás suavizan a costa de nitidez.",
  "Sceglie il filtro per ingrandire l'immagine Wii U. Nearest è nitido e pixellato; gli altri sono più morbidi ma meno definiti.",
  "Escolhe o filtro ao ampliar a imagem da Wii U. O mais próximo é nítido e pixelizado; os outros suavizam sacrificando nitidez."},
 {"Selects the filter used when an image must be reduced. The choice changes sharpness and aliasing but does not change the game's internal rendering resolution.",
  "Choisit le filtre de réduction d'image. Il modifie la netteté et l'aliasing, mais pas la résolution de rendu interne du jeu.",
  "Wählt den Filter zum Verkleinern eines Bilds. Er beeinflusst Schärfe und Aliasing, nicht die interne Renderauflösung.",
  "Elige el filtro al reducir una imagen. Cambia la nitidez y el aliasing, no la resolución interna del juego.",
  "Sceglie il filtro per ridurre un'immagine. Cambia nitidezza e aliasing, non la risoluzione interna del gioco.",
  "Escolhe o filtro ao reduzir uma imagem. Altera a nitidez e o serrilhado, não a resolução interna do jogo."},
 {"Keep aspect ratio preserves the game's intended shape and may leave borders. Stretch fills the output but can distort the image.",
  "Conserver le format respecte les proportions du jeu et peut laisser des bordures. Étirer remplit l'écran mais peut déformer l'image.",
  "Seitenverhältnis beibehalten bewahrt die Form und kann Ränder lassen. Strecken füllt den Bildschirm, kann aber verzerren.",
  "Mantener proporción conserva la forma y puede dejar bordes. Estirar llena la pantalla, pero puede deformar la imagen.",
  "Mantieni proporzioni conserva la forma e può lasciare bordi. Allunga riempie lo schermo ma può deformare l'immagine.",
  "Manter proporção preserva a forma e pode deixar margens. Esticar preenche o ecrã, mas pode deformar a imagem."},
 {"Chooses whether to show the TV view, the GamePad view, or both views in a split layout. Composite layouts reduce the space available to each screen.",
  "Choisit la vue TV, la vue GamePad ou les deux en écran partagé. Les dispositions combinées réduisent l'espace de chaque écran.",
  "Wählt TV-, GamePad- oder geteilte Ansicht. Kombinierte Layouts verkleinern den Platz für jeden Bildschirm.",
  "Elige la vista de TV, la del GamePad o ambas divididas. Los diseños combinados reducen el espacio de cada pantalla.",
  "Sceglie la vista TV, GamePad o entrambe divise. I layout combinati riducono lo spazio di ogni schermo.",
  "Escolhe a vista da TV, do GamePad ou ambas divididas. Os esquemas combinados reduzem o espaço de cada ecrã."},
 {"Sets the volume of the emulated Wii U TV audio stream sent to the Switch output.",
  "Règle le volume du flux audio TV Wii U émulé envoyé à la sortie de la Switch.",
  "Stellt die Lautstärke des emulierten Wii-U-TV-Audiostreams am Switch-Ausgang ein.",
  "Ajusta el volumen del flujo de audio de TV de Wii U enviado a la salida de Switch.",
  "Regola il volume del flusso audio TV Wii U inviato all'uscita Switch.",
  "Define o volume do áudio da TV Wii U emulada enviado para a saída da Switch."},
 {"Enables the separate audio stream that Wii U software sends to the GamePad. Disable it when duplicated TV and GamePad sound is unwanted.",
  "Active le flux audio séparé envoyé au GamePad. Désactivez-le si le son TV et GamePad est dupliqué de façon indésirable.",
  "Aktiviert den separaten GamePad-Audiostream. Deaktivieren, wenn TV- und GamePad-Ton unerwünscht doppelt ausgegeben werden.",
  "Activa el audio separado enviado al GamePad. Desactívalo si no quieres sonido duplicado de TV y GamePad.",
  "Attiva l'audio separato inviato al GamePad. Disattivalo se il suono TV e GamePad viene duplicato.",
  "Ativa o áudio separado enviado ao GamePad. Desative se não quiser som duplicado da TV e do GamePad."},
 {"Sets the volume of the emulated GamePad audio stream when GamePad audio is enabled.",
  "Règle le volume du flux audio du GamePad émulé lorsque son audio est activé.",
  "Stellt die Lautstärke des emulierten GamePad-Audiostreams ein, wenn GamePad-Audio aktiv ist.",
  "Ajusta el volumen del audio del GamePad emulado cuando está activado.",
  "Regola il volume dell'audio del GamePad emulato quando è attivo.",
  "Define o volume do áudio do GamePad emulado quando está ativado."},
 {"Controls Cemu's audio buffer target. Lower values reduce latency but can crackle when emulation is uneven; higher values are more stable but respond later.",
  "Contrôle la taille cible du tampon audio. Une valeur basse réduit la latence mais peut craquer ; une valeur haute est plus stable mais réagit plus tard.",
  "Steuert den Ziel-Audiopuffer. Niedrige Werte verringern Latenz, können aber knistern; hohe Werte sind stabiler, reagieren jedoch später.",
  "Controla el búfer de audio. Valores bajos reducen latencia pero pueden producir chasquidos; valores altos son más estables pero responden más tarde.",
  "Controlla il buffer audio. Valori bassi riducono la latenza ma possono causare crepitii; valori alti sono più stabili ma rispondono più tardi.",
  "Controla o buffer de áudio. Valores baixos reduzem a latência mas podem causar estalidos; valores altos são mais estáveis mas respondem mais tarde."},
 {"Generates one intermediate display frame for each real frame to target a smoother 2x presentation. It does not increase emulation speed and may add artifacts or latency. Vulkan only.",
  "Génère une image intermédiaire par image réelle pour un affichage 2x plus fluide. Cela n'accélère pas l'émulation et peut ajouter artefacts ou latence. Vulkan uniquement.",
  "Erzeugt pro echtem Bild ein Zwischenbild für eine flüssigere 2x-Darstellung. Die Emulation wird nicht schneller; Artefakte oder Latenz sind möglich. Nur Vulkan.",
  "Genera un fotograma intermedio por cada real para una presentación 2x más fluida. No acelera la emulación y puede añadir artefactos o latencia. Solo Vulkan.",
  "Genera un fotogramma intermedio per ogni fotogramma reale, per una presentazione 2x più fluida. Non accelera l'emulazione e può aggiungere artefatti o latenza. Solo Vulkan.",
  "Gera um fotograma intermédio por cada real para apresentação 2x mais fluida. Não acelera a emulação e pode adicionar artefactos ou latência. Apenas Vulkan."},
 {"Sets the resolution used for optical-flow analysis. Half resolution can retain more motion detail but costs more GPU time and memory; Quarter is recommended on Switch.",
  "Règle la résolution de l'analyse du flux optique. La demi-résolution conserve plus de détails mais coûte davantage en GPU et mémoire ; le quart est recommandé sur Switch.",
  "Legt die Auflösung der Optical-Flow-Analyse fest. Halb behält mehr Bewegungsdetails, benötigt aber mehr GPU-Zeit und Speicher; Viertel wird auf Switch empfohlen.",
  "Define la resolución del flujo óptico. Media conserva más detalle de movimiento pero usa más GPU y memoria; se recomienda un cuarto en Switch.",
  "Imposta la risoluzione del flusso ottico. Metà conserva più dettagli ma usa più GPU e memoria; un quarto è consigliato su Switch.",
  "Define a resolução do fluxo ótico. Metade preserva mais detalhe mas usa mais GPU e memória; um quarto é recomendado na Switch."},
 {"Uses LSFG's lighter performance-oriented processing path. Disable it only when you prefer image quality and have enough GPU headroom.",
  "Utilise le traitement LSFG allégé orienté performances. Désactivez-le seulement si vous privilégiez la qualité et disposez d'une marge GPU suffisante.",
  "Nutzt den leichteren, leistungsorientierten LSFG-Pfad. Nur deaktivieren, wenn Bildqualität wichtiger ist und genug GPU-Reserve besteht.",
  "Usa la ruta ligera de LSFG orientada al rendimiento. Desactívala solo si priorizas calidad y tienes margen de GPU.",
  "Usa il percorso LSFG leggero orientato alle prestazioni. Disattivalo solo se preferisci la qualità e hai margine GPU.",
  "Usa o modo LSFG mais leve orientado ao desempenho. Desative apenas se preferir qualidade e tiver margem de GPU."},
 {"Shows Cemu's frame-rate counter during gameplay.",
  "Affiche le compteur de fréquence d'images de Cemu pendant le jeu.",
  "Zeigt Cemus Bildratenzähler während des Spiels.",
  "Muestra el contador de fotogramas de Cemu durante el juego.",
  "Mostra il contatore dei fotogrammi di Cemu durante il gioco.",
  "Mostra o contador de fotogramas do Cemu durante o jogo."},
 {"Chooses where Cemu's on-screen performance overlay is drawn. Disabled hides the overlay and its FPS counter.",
  "Choisit où afficher la surimpression de performances. Désactivé masque la surimpression et le compteur FPS.",
  "Wählt die Position der Leistungsanzeige. Deaktiviert blendet Overlay und FPS-Zähler aus.",
  "Elige dónde se muestra la superposición de rendimiento. Desactivado oculta la superposición y el contador FPS.",
  "Sceglie dove mostrare la sovrimpressione delle prestazioni. Disattivato nasconde overlay e contatore FPS.",
  "Escolhe onde mostrar a sobreposição de desempenho. Desativado oculta-a e o contador de FPS."},
 {"Shows a notification while Cemu is compiling shaders so brief stutter or temporarily missing effects can be identified.",
  "Affiche une notification pendant la compilation des shaders afin d'identifier les saccades ou effets brièvement absents.",
  "Zeigt während der Shader-Kompilierung einen Hinweis, damit kurze Ruckler oder fehlende Effekte erkannt werden.",
  "Muestra un aviso al compilar shaders para identificar tirones o efectos ausentes temporalmente.",
  "Mostra un avviso durante la compilazione degli shader per riconoscere scatti o effetti temporaneamente mancanti.",
  "Mostra um aviso durante a compilação de shaders para identificar soluços ou efeitos temporariamente em falta."},
 {"Selects the Wii U controller type presented to the game. Wii U GamePad is required by games that use its screen, microphone, or touch features.",
  "Sélectionne le type de manette Wii U présenté au jeu. Le GamePad est requis par les jeux utilisant son écran, son microphone ou le tactile.",
  "Wählt den emulierten Wii-U-Controller. Das GamePad ist für Spiele erforderlich, die Bildschirm, Mikrofon oder Touch nutzen.",
  "Selecciona el mando Wii U presentado al juego. El GamePad es obligatorio para juegos que usan pantalla, micrófono o táctil.",
  "Seleziona il controller Wii U presentato al gioco. Il GamePad è richiesto dai giochi che usano schermo, microfono o tocco.",
  "Seleciona o comando Wii U apresentado ao jogo. O GamePad é necessário para jogos que usam ecrã, microfone ou toque."},
 {"Sets how many active emulated controller slots Cemu creates for local multiplayer.",
  "Définit le nombre d'emplacements de manettes émulées actifs pour le multijoueur local.",
  "Legt die Zahl aktiver emulierter Controller-Steckplätze für lokalen Mehrspieler fest.",
  "Define cuántas ranuras de mandos emulados hay activas para multijugador local.",
  "Imposta quanti slot di controller emulati sono attivi per il multigiocatore locale.",
  "Define quantas ranhuras de comandos emulados estão ativas para multijogador local."},
 {"Forwards supported Wii U controller vibration effects to the connected Switch controller.",
  "Transmet les vibrations Wii U prises en charge à la manette Switch connectée.",
  "Leitet unterstützte Wii-U-Vibrationen an den verbundenen Switch-Controller weiter.",
  "Envía los efectos de vibración compatibles al mando Switch conectado.",
  "Invia gli effetti di vibrazione supportati al controller Switch collegato.",
  "Envia os efeitos de vibração suportados para o comando Switch ligado."},
 {"Sets how far an analog stick must move before Cemu accepts input. Raise it to prevent drift; lower it for quicker response.",
  "Définit la course nécessaire avant que Cemu accepte le stick. Augmentez-la contre la dérive ; réduisez-la pour une réponse plus rapide.",
  "Legt fest, wie weit ein Stick bewegt werden muss. Erhöhen gegen Drift; verringern für schnellere Reaktion.",
  "Define cuánto debe moverse el stick para registrar entrada. Súbelo para evitar deriva; bájalo para responder antes.",
  "Imposta quanto deve muoversi lo stick per registrare l'input. Aumenta contro il drift; riduci per una risposta più rapida.",
  "Define quanto o analógico deve mover-se para registar entrada. Aumente contra drift; reduza para resposta mais rápida."},
 {"Enables Cemu's virtual Skylanders Portal for games that communicate with that USB accessory.",
  "Active le portail Skylanders virtuel de Cemu pour les jeux qui utilisent cet accessoire USB.",
  "Aktiviert Cemus virtuelles Skylanders-Portal für Spiele, die dieses USB-Zubehör nutzen.",
  "Activa el Portal de Skylanders virtual de Cemu para juegos que usan ese accesorio USB.",
  "Attiva il Portale Skylanders virtuale di Cemu per i giochi che usano tale accessorio USB.",
  "Ativa o Portal Skylanders virtual do Cemu para jogos que usam esse acessório USB."},
 {"Enables Cemu's virtual Disney Infinity Base for games that communicate with that USB accessory.",
  "Active la base Disney Infinity virtuelle de Cemu pour les jeux qui utilisent cet accessoire USB.",
  "Aktiviert Cemus virtuelle Disney-Infinity-Basis für Spiele, die dieses USB-Zubehör nutzen.",
  "Activa la Base Disney Infinity virtual de Cemu para juegos que usan ese accesorio USB.",
  "Attiva la Base Disney Infinity virtuale di Cemu per i giochi che usano tale accessorio USB.",
  "Ativa a Base Disney Infinity virtual do Cemu para jogos que usam esse acessório USB."},
 {"Enables Cemu's virtual LEGO Dimensions Toy Pad for games that communicate with that USB accessory.",
  "Active le Toy Pad LEGO Dimensions virtuel de Cemu pour les jeux qui utilisent cet accessoire USB.",
  "Aktiviert Cemus virtuelles LEGO-Dimensions-Toypad für Spiele, die dieses USB-Zubehör nutzen.",
  "Activa el Toy Pad de LEGO Dimensions virtual de Cemu para juegos que usan ese accesorio USB.",
  "Attiva il Toy Pad LEGO Dimensions virtuale di Cemu per i giochi che usano tale accessorio USB.",
  "Ativa o Toy Pad LEGO Dimensions virtual do Cemu para jogos que usam esse acessório USB."},
 {"Sets the language reported by the emulated Wii U console. Auto follows the Switch system language when Cemu supports it; games may need to be restarted after a change.",
  "Définit la langue signalée par la Wii U émulée. Auto suit la langue système de la Switch si elle est prise en charge ; redémarrez le jeu après modification.",
  "Legt die Sprache der emulierten Wii U fest. Auto folgt, sofern unterstützt, der Switch-Systemsprache; Spiele müssen eventuell neu gestartet werden.",
  "Define el idioma de la Wii U emulada. Automático sigue el idioma de Switch si es compatible; puede ser necesario reiniciar el juego.",
  "Imposta la lingua della Wii U emulata. Auto segue la lingua di Switch se supportata; potrebbe servire riavviare il gioco.",
  "Define o idioma da Wii U emulada. Automático segue o idioma da Switch quando suportado; pode ser necessário reiniciar o jogo."},
 {"Changes the SDL launcher's background and visual theme. It does not affect gameplay rendering.",
  "Change l'arrière-plan et le thème visuel du lanceur SDL. Cela n'affecte pas le rendu des jeux.",
  "Ändert Hintergrund und Design des SDL-Launchers. Die Spieldarstellung wird nicht beeinflusst.",
  "Cambia el fondo y el tema visual del lanzador SDL. No afecta al renderizado de los juegos.",
  "Cambia sfondo e tema visivo del launcher SDL. Non influisce sul rendering dei giochi.",
  "Altera o fundo e o tema visual do launcher SDL. Não afeta a renderização dos jogos."},
 {"Selects the language used by the SDL launcher. System follows the Switch console language; technical emulator names and identifiers remain unchanged for accuracy.",
  "Sélectionne la langue du lanceur SDL. Système suit la langue de la console Switch ; les noms techniques et identifiants de l'émulateur restent inchangés pour garantir leur précision.",
  "Wählt die Sprache des SDL-Launchers. System folgt der Switch-Konsolensprache; technische Emulatornamen und Kennungen bleiben zur Genauigkeit unverändert.",
  "Selecciona el idioma del lanzador SDL. Sistema sigue el idioma de la consola Switch; los nombres técnicos e identificadores del emulador no se traducen para conservar su precisión.",
  "Seleziona la lingua del launcher SDL. Sistema segue la lingua della console Switch; nomi tecnici e identificatori dell'emulatore restano invariati per precisione.",
  "Seleciona o idioma do launcher SDL. Sistema segue o idioma da consola Switch; nomes técnicos e identificadores do emulador permanecem inalterados para maior precisão."},
 {"Sets how many game covers are displayed across each library row. More columns make each cover smaller.",
  "Définit le nombre de jaquettes par ligne. Davantage de colonnes réduit leur taille.",
  "Legt die Zahl der Cover pro Reihe fest. Mehr Spalten machen jedes Cover kleiner.",
  "Define cuántas carátulas hay por fila. Más columnas hacen cada carátula más pequeña.",
  "Imposta quante copertine compaiono per riga. Più colonne rendono ogni copertina più piccola.",
  "Define quantas capas aparecem por linha. Mais colunas tornam cada capa menor."},
 {"Sets how many rows of game covers are displayed on each library page. More rows make each cover smaller.",
  "Définit le nombre de lignes de jaquettes par page. Davantage de lignes réduit leur taille.",
  "Legt die Zahl der Cover-Reihen pro Seite fest. Mehr Reihen machen jedes Cover kleiner.",
  "Define cuántas filas de carátulas hay por página. Más filas hacen cada carátula más pequeña.",
  "Imposta quante righe di copertine compaiono per pagina. Più righe rendono ogni copertina più piccola.",
  "Define quantas linhas de capas aparecem por página. Mais linhas tornam cada capa menor."},
 {"Shows or hides game names below their cover artwork in the launcher library.",
  "Affiche ou masque le nom des jeux sous leur jaquette dans la bibliothèque.",
  "Blendet Spielnamen unter den Covern in der Bibliothek ein oder aus.",
  "Muestra u oculta los nombres de los juegos bajo sus carátulas.",
  "Mostra o nasconde i nomi dei giochi sotto le copertine.",
  "Mostra ou oculta os nomes dos jogos sob as capas."},
 {"Shows or hides the region flag in the top-left corner of each game cover.",
  "Affiche ou masque le drapeau de région dans le coin supérieur gauche de chaque jaquette.",
  "Blendet die Regionsflagge oben links auf jedem Spielcover ein oder aus.",
  "Muestra u oculta la bandera de región en la esquina superior izquierda de cada carátula.",
  "Mostra o nasconde la bandiera regionale nell'angolo superiore sinistro di ogni copertina.",
  "Mostra ou oculta a bandeira de região no canto superior esquerdo de cada capa."},
 {"Shows or hides the square badge on games that have per-game settings. The settings themselves are not changed.",
  "Affiche ou masque l'indicateur carré sur les jeux ayant des paramètres par jeu. Les paramètres eux-mêmes ne sont pas modifiés.",
  "Blendet die quadratische Markierung bei Spielen mit spielspezifischen Einstellungen ein oder aus. Die Einstellungen selbst werden nicht geändert.",
  "Muestra u oculta el indicador cuadrado en los juegos con ajustes por juego. Los ajustes no se modifican.",
  "Mostra o nasconde l'indicatore quadrato sui giochi con impostazioni specifiche. Le impostazioni non vengono modificate.",
  "Mostra ou oculta o indicador quadrado nos jogos com definições por jogo. As definições não são alteradas."},
 {"Enables launcher transitions, moving highlights, and animated theme effects.",
  "Active les transitions, les sélections mobiles et les effets de thème animés du lanceur.",
  "Aktiviert Launcher-Übergänge, bewegte Hervorhebungen und animierte Designeffekte.",
  "Activa transiciones, resaltados móviles y efectos animados del lanzador.",
  "Attiva transizioni, evidenziazioni mobili ed effetti animati del launcher.",
  "Ativa transições, destaques móveis e efeitos animados do launcher."},
 {"Enables navigation, confirmation, and back sound effects in the SDL launcher.",
  "Active les effets sonores de navigation, validation et retour du lanceur SDL.",
  "Aktiviert Navigations-, Bestätigungs- und Zurück-Sounds im SDL-Launcher.",
  "Activa los sonidos de navegación, confirmación y regreso del lanzador SDL.",
  "Attiva i suoni di navigazione, conferma e ritorno del launcher SDL.",
  "Ativa os sons de navegação, confirmação e voltar do launcher SDL."},
 {"Checks for a newer Cemu-nx release when the SDL launcher starts. The result appears in Launcher settings.",
  "Recherche une nouvelle version de Cemu-nx au démarrage du lanceur SDL. Le résultat apparaît dans ses paramètres.",
  "Sucht beim Start des SDL-Launchers nach einer neueren Cemu-nx-Version. Das Ergebnis erscheint in den Launcher-Einstellungen.",
  "Busca una versión nueva de Cemu-nx al iniciar el lanzador SDL. El resultado aparece en sus ajustes.",
  "Cerca una nuova versione di Cemu-nx all'avvio del launcher SDL. Il risultato appare nelle impostazioni.",
  "Procura uma nova versão do Cemu-nx ao iniciar o launcher SDL. O resultado aparece nas definições."},
 {"Opens the press-to-bind screen for every Wii U controller input. Select a control, then press the Switch button or trigger that should activate it.",
  "Ouvre l'écran d'assignation de chaque entrée Wii U. Sélectionnez une commande, puis appuyez sur le bouton ou la gâchette Switch à lui attribuer.",
  "Öffnet die Belegung für jede Wii-U-Eingabe. Steuerung auswählen und dann die gewünschte Switch-Taste oder den Trigger drücken.",
  "Abre la asignación de cada entrada Wii U. Selecciona un control y pulsa el botón o gatillo de Switch que quieras asignar.",
  "Apre l'assegnazione di ogni input Wii U. Seleziona un comando e premi il pulsante o grilletto Switch da assegnare.",
  "Abre o mapeamento de cada entrada Wii U. Selecione um controlo e prima o botão ou gatilho Switch a atribuir."},
 {"Shows whether the Lossless Scaling frame-generation library is installed. LSFG cannot be enabled until Lossless.dll is copied to sdmc:/switch/cemu/lsfg/.",
  "Indique si la bibliothèque de génération d'images Lossless Scaling est installée. LSFG reste indisponible tant que Lossless.dll n'est pas copié dans sdmc:/switch/cemu/lsfg/.",
  "Zeigt, ob die Lossless-Scaling-Bibliothek installiert ist. LSFG kann erst aktiviert werden, wenn Lossless.dll nach sdmc:/switch/cemu/lsfg/ kopiert wurde.",
  "Indica si está instalada la biblioteca Lossless Scaling. LSFG no puede activarse hasta copiar Lossless.dll en sdmc:/switch/cemu/lsfg/.",
  "Indica se la libreria Lossless Scaling è installata. LSFG non può essere attivato finché Lossless.dll non viene copiato in sdmc:/switch/cemu/lsfg/.",
  "Indica se a biblioteca Lossless Scaling está instalada. LSFG só pode ser ativado após copiar Lossless.dll para sdmc:/switch/cemu/lsfg/."},
 // Exact English fallbacks for launcher-owned diagnostics which use standardized
 // API, file-system, account, or recovery wording.  They are catalogued
 // explicitly so they cannot accidentally pass through the dynamic-text path.
 {" cache file(s) could not be removed."," cache file(s) could not be removed."," cache file(s) could not be removed."," cache file(s) could not be removed."," cache file(s) could not be removed."," cache file(s) could not be removed."},
 {"A SteamGridDB API key is required","A SteamGridDB API key is required","A SteamGridDB API key is required","A SteamGridDB API key is required","A SteamGridDB API key is required","A SteamGridDB API key is required"},
 {"Account","Account","Account","Account","Account","Account"},
 {"Account used by the SMB server. Leave blank for guest access.","Account used by the SMB server. Leave blank for guest access.","Account used by the SMB server. Leave blank for guest access.","Account used by the SMB server. Leave blank for guest access.","Account used by the SMB server. Leave blank for guest access.","Account used by the SMB server. Leave blank for guest access."},
 {"Add network_services.xml to the Cemu directory.","Add network_services.xml to the Cemu directory.","Add network_services.xml to the Cemu directory.","Add network_services.xml to the Cemu directory.","Add network_services.xml to the Cemu directory.","Add network_services.xml to the Cemu directory."},
 {"Add the key to sdmc:/switch/cemu/keys.txt.","Add the key to sdmc:/switch/cemu/keys.txt.","Add the key to sdmc:/switch/cemu/keys.txt.","Add the key to sdmc:/switch/cemu/keys.txt.","Add the key to sdmc:/switch/cemu/keys.txt.","Add the key to sdmc:/switch/cemu/keys.txt."},
 {"All covers already downloaded","All covers already downloaded","All covers already downloaded","All covers already downloaded","All covers already downloaded","All covers already downloaded"},
 {"All folders are scanned and passed to Cemu","All folders are scanned and passed to Cemu","All folders are scanned and passed to Cemu","All folders are scanned and passed to Cemu","All folders are scanned and passed to Cemu","All folders are scanned and passed to Cemu"},
 {"Already in the games folder","Already in the games folder","Already in the games folder","Already in the games folder","Already in the games folder","Already in the games folder"},
 {"An item with that name already exists.","An item with that name already exists.","An item with that name already exists.","An item with that name already exists.","An item with that name already exists.","An item with that name already exists."},
 {"Artwork search failed","Échec de la recherche d’images","Bildsuche fehlgeschlagen","Falló la búsqueda de imágenes","Ricerca immagini non riuscita","Falha na pesquisa de imagens"},
 {"Building + installing forwarder...","Building + installing forwarder...","Building + installing forwarder...","Building + installing forwarder...","Building + installing forwarder...","Building + installing forwarder..."},
 {"Cache cleanup failed","Cache cleanup failed","Cache cleanup failed","Cache cleanup failed","Cache cleanup failed","Cache cleanup failed"},
 {"Cancelling...","Cancelling...","Cancelling...","Cancelling...","Cancelling...","Cancelling..."},
 {"Cemu Update","Cemu Update","Cemu Update","Cemu Update","Cemu Update","Cemu Update"},
 {"Cemu configuration could not be updated safely.","Cemu configuration could not be updated safely.","Cemu configuration could not be updated safely.","Cemu configuration could not be updated safely.","Cemu configuration could not be updated safely.","Cemu configuration could not be updated safely."},
 {"Cemu will rebuild them the next time the game runs.","Cemu will rebuild them the next time the game runs.","Cemu will rebuild them the next time the game runs.","Cemu will rebuild them the next time the game runs.","Cemu will rebuild them the next time the game runs.","Cemu will rebuild them the next time the game runs."},
 {"Cemu/keys.txt does not contain a Wii U disc key.","Cemu/keys.txt does not contain a Wii U disc key.","Cemu/keys.txt does not contain a Wii U disc key.","Cemu/keys.txt does not contain a Wii U disc key.","Cemu/keys.txt does not contain a Wii U disc key.","Cemu/keys.txt does not contain a Wii U disc key."},
 {"Check free SD space and file permissions.","Check free SD space and file permissions.","Check free SD space and file permissions.","Check free SD space and file permissions.","Check free SD space and file permissions.","Check free SD space and file permissions."},
 {"Check the connection and try again.","Check the connection and try again.","Check the connection and try again.","Check the connection and try again.","Check the connection and try again.","Check the connection and try again."},
 {"Choose an icon","Choisir une icône","Symbol auswählen","Elegir un icono","Scegli un’icona","Escolher um ícone"},
 {"Choose another destination or rename the folder first.","Choose another destination or rename the folder first.","Choose another destination or rename the folder first.","Choose another destination or rename the folder first.","Choose another destination or rename the folder first.","Choose another destination or rename the folder first."},
 {"Choose cover artwork","Choisir une jaquette","Cover auswählen","Elegir una carátula","Scegli una copertina","Escolher uma capa"},
 {"Choose matching title","Choose matching title","Choose matching title","Choose matching title","Choose matching title","Choose matching title"},
 {"Clear shader caches?","Vider les caches de shaders ?","Shader-Caches leeren?","¿Vaciar las cachés de shaders?","Svuotare le cache degli shader?","Limpar as caches de shaders?"},
 {"Close files using this drive before ejecting.","Close files using this drive before ejecting.","Close files using this drive before ejecting.","Close files using this drive before ejecting.","Close files using this drive before ejecting.","Close files using this drive before ejecting."},
 {"Collection already exists","Cette collection existe déjà","Sammlung ist bereits vorhanden","La colección ya existe","La raccolta esiste già","A coleção já existe"},
 {"Collection name","Nom de la collection","Sammlungsname","Nombre de la colección","Nome della raccolta","Nome da coleção"},
 {"Connecting USB storage","Connecting USB storage","Connecting USB storage","Connecting USB storage","Connecting USB storage","Connecting USB storage"},
 {"Connection preview","Connection preview","Connection preview","Connection preview","Connection preview","Connection preview"},
 {"Control assigned","Commande assignée","Steuerung zugewiesen","Control asignado","Comando assegnato","Controlo atribuído"},
 {"Copied to clipboard","Copied to clipboard","Copied to clipboard","Copied to clipboard","Copied to clipboard","Copied to clipboard"},
 {"Copy an existing Wii U account.dat into:","Copy an existing Wii U account.dat into:","Copy an existing Wii U account.dat into:","Copy an existing Wii U account.dat into:","Copy an existing Wii U account.dat into:","Copy an existing Wii U account.dat into:"},
 {"Copy it to sdmc:/switch/cemu/lsfg/Lossless.dll","Copy it to sdmc:/switch/cemu/lsfg/Lossless.dll","Copy it to sdmc:/switch/cemu/lsfg/Lossless.dll","Copy it to sdmc:/switch/cemu/lsfg/Lossless.dll","Copy it to sdmc:/switch/cemu/lsfg/Lossless.dll","Copy it to sdmc:/switch/cemu/lsfg/Lossless.dll"},
 {"Could not check destination","Could not check destination","Could not check destination","Could not check destination","Could not check destination","Could not check destination"},
 {"Could not create games folder","Could not create games folder","Could not create games folder","Could not create games folder","Could not create games folder","Could not create games folder"},
 {"Could not install the emulator to the SD card.","Could not install the emulator to the SD card.","Could not install the emulator to the SD card.","Could not install the emulator to the SD card.","Could not install the emulator to the SD card.","Could not install the emulator to the SD card."},
 {"Could not preserve the existing destination.","Could not preserve the existing destination.","Could not preserve the existing destination.","Could not preserve the existing destination.","Could not preserve the existing destination.","Could not preserve the existing destination."},
 {"Could not recover previous install","Could not recover previous install","Could not recover previous install","Could not recover previous install","Could not recover previous install","Could not recover previous install"},
 {"Could not save graphics packs","Could not save graphics packs","Could not save graphics packs","Could not save graphics packs","Could not save graphics packs","Could not save graphics packs"},
 {"Cover download failed","Cover download failed","Cover download failed","Cover download failed","Cover download failed","Cover download failed"},
 {"Cover downloaded","Jaquette téléchargée","Cover heruntergeladen","Carátula descargada","Copertina scaricata","Capa transferida"},
 {"Cover search failed","Cover search failed","Cover search failed","Cover search failed","Cover search failed","Cover search failed"},
 {"Create HOME shortcut","Créer un raccourci HOME","HOME-Verknüpfung erstellen","Crear acceso directo HOME","Crea collegamento HOME","Criar atalho HOME"},
 {"Create shortcut","Create shortcut","Create shortcut","Create shortcut","Create shortcut","Create shortcut"},
 {"Creating HOME shortcut","Creating HOME shortcut","Creating HOME shortcut","Creating HOME shortcut","Creating HOME shortcut","Creating HOME shortcut"},
 {"Credential readiness","Credential readiness","Credential readiness","Credential readiness","Credential readiness","Credential readiness"},
 {"Custom SteamGridDB search","Custom SteamGridDB search","Custom SteamGridDB search","Custom SteamGridDB search","Custom SteamGridDB search","Custom SteamGridDB search"},
 {"Custom service needs configuration","Custom service needs configuration","Custom service needs configuration","Custom service needs configuration","Custom service needs configuration","Custom service needs configuration"},
 {"Delete collection?","Supprimer la collection ?","Sammlung löschen?","¿Eliminar la colección?","Eliminare la raccolta?","Eliminar a coleção?"},
 {"Delete failed","Delete failed","Delete failed","Delete failed","Delete failed","Delete failed"},
 {"Delete game?","Supprimer le jeu ?","Spiel löschen?","¿Eliminar el juego?","Eliminare il gioco?","Eliminar o jogo?"},
 {"Destination is not a file","Destination is not a file","Destination is not a file","Destination is not a file","Destination is not a file","Destination is not a file"},
 {"Disc key required","Disc key required","Disc key required","Disc key required","Disc key required","Disc key required"},
 {"Display name required","Display name required","Display name required","Display name required","Display name required","Display name required"},
 {"Do not include a drive letter or smb:// prefix.","Do not include a drive letter or smb:// prefix.","Do not include a drive letter or smb:// prefix.","Do not include a drive letter or smb:// prefix.","Do not include a drive letter or smb:// prefix.","Do not include a drive letter or smb:// prefix."},
 {"Download latest packs","Download latest packs","Download latest packs","Download latest packs","Download latest packs","Download latest packs"},
 {"Downloading latest graphic packs...","Downloading latest graphic packs...","Downloading latest graphic packs...","Downloading latest graphic packs...","Downloading latest graphic packs...","Downloading latest graphic packs..."},
 {"Downloading selected cover...","Downloading selected cover...","Downloading selected cover...","Downloading selected cover...","Downloading selected cover...","Downloading selected cover..."},
 {"Edit / toggle","Edit / toggle","Edit / toggle","Edit / toggle","Edit / toggle","Edit / toggle"},
 {"Emulator missing - copy cemu.nro into sdmc:/switch/cemu/","Emulator missing - copy cemu.nro into sdmc:/switch/cemu/","Emulator missing - copy cemu.nro into sdmc:/switch/cemu/","Emulator missing - copy cemu.nro into sdmc:/switch/cemu/","Emulator missing - copy cemu.nro into sdmc:/switch/cemu/","Emulator missing - copy cemu.nro into sdmc:/switch/cemu/"},
 {"Emulator setup failed","Emulator setup failed","Emulator setup failed","Emulator setup failed","Emulator setup failed","Emulator setup failed"},
 {"Preparing emulator...","Préparation de l'émulateur...","Emulator wird vorbereitet...","Preparando el emulador...","Preparazione dell'emulatore...","A preparar o emulador..."},
 {"The embedded Cemu core could not be prepared.","Le cœur Cemu intégré n'a pas pu être préparé.","Der eingebettete Cemu-Kern konnte nicht vorbereitet werden.","No se pudo preparar el núcleo Cemu integrado.","Impossibile preparare il core Cemu integrato.","Não foi possível preparar o núcleo Cemu integrado."},
 {"The SD card may be full or write-protected.","La carte SD est peut-être pleine ou protégée en écriture.","Die SD-Karte ist möglicherweise voll oder schreibgeschützt.","Puede que la tarjeta SD esté llena o protegida contra escritura.","La scheda SD potrebbe essere piena o protetta da scrittura.","O cartão SD pode estar cheio ou protegido contra escrita."},
 {"Free some space and try launching the game again.","Libérez de l'espace puis relancez le jeu.","Gib Speicherplatz frei und starte das Spiel erneut.","Libera espacio e intenta iniciar el juego de nuevo.","Libera spazio e prova ad avviare di nuovo il gioco.","Liberte espaço e tente iniciar o jogo novamente."},
 {"Enter a name used to identify this share in Cemu.","Enter a name used to identify this share in Cemu.","Enter a name used to identify this share in Cemu.","Enter a name used to identify this share in Cemu.","Enter a name used to identify this share in Cemu.","Enter a name used to identify this share in Cemu."},
 {"Enter a share name, optionally followed by folders.","Enter a share name, optionally followed by folders.","Enter a share name, optionally followed by folders.","Enter a share name, optionally followed by folders.","Enter a share name, optionally followed by folders.","Enter a share name, optionally followed by folders."},
 {"Enter only a host name or IP address.","Enter only a host name or IP address.","Enter only a host name or IP address.","Enter only a host name or IP address.","Enter only a host name or IP address.","Enter only a host name or IP address."},
 {"Enter the network host only. Do not include smb:// or a folder.","Enter the network host only. Do not include smb:// or a folder.","Enter the network host only. Do not include smb:// or a folder.","Enter the network host only. Do not include smb:// or a folder.","Enter the network host only. Do not include smb:// or a folder.","Enter the network host only. Do not include smb:// or a folder."},
 {"Enter the share and an optional folder path inside it.","Enter the share and an optional folder path inside it.","Enter the share and an optional folder path inside it.","Enter the share and an optional folder path inside it.","Enter the share and an optional folder path inside it.","Enter the share and an optional folder path inside it."},
 {"Example: 192.168.1.20","Example: 192.168.1.20","Example: 192.168.1.20","Example: 192.168.1.20","Example: 192.168.1.20","Example: 192.168.1.20"},
 {"Example: 192.168.1.20 or NAS.local","Example: 192.168.1.20 or NAS.local","Example: 192.168.1.20 or NAS.local","Example: 192.168.1.20 or NAS.local","Example: 192.168.1.20 or NAS.local","Example: 192.168.1.20 or NAS.local"},
 {"Example: Living room NAS","Example: Living room NAS","Example: Living room NAS","Example: Living room NAS","Example: Living room NAS","Example: Living room NAS"},
 {"Example: WORKGROUP, or leave blank","Example: WORKGROUP, or leave blank","Example: WORKGROUP, or leave blank","Example: WORKGROUP, or leave blank","Example: WORKGROUP, or leave blank","Example: WORKGROUP, or leave blank"},
 {"Extracted game folders are not deleted automatically.","Extracted game folders are not deleted automatically.","Extracted game folders are not deleted automatically.","Extracted game folders are not deleted automatically.","Extracted game folders are not deleted automatically.","Extracted game folders are not deleted automatically."},
 {"Fetching icons from SteamGridDB...","Fetching icons from SteamGridDB...","Fetching icons from SteamGridDB...","Fetching icons from SteamGridDB...","Fetching icons from SteamGridDB...","Fetching icons from SteamGridDB..."},
 {"File options","Options du fichier","Dateioptionen","Opciones del archivo","Opzioni file","Opções do ficheiro"},
 {"File transfer","Transfert de fichier","Dateiübertragung","Transferencia de archivos","Trasferimento file","Transferência de ficheiros"},
 {"Folder already added","Dossier déjà ajouté","Ordner bereits hinzugefügt","Carpeta ya añadida","Cartella già aggiunta","Pasta já adicionada"},
 {"Folder already exists","Folder already exists","Folder already exists","Folder already exists","Folder already exists","Folder already exists"},
 {"Folder deletion disabled","Folder deletion disabled","Folder deletion disabled","Folder deletion disabled","Folder deletion disabled","Folder deletion disabled"},
 {"Folder unavailable","Folder unavailable","Folder unavailable","Folder unavailable","Folder unavailable","Folder unavailable"},
 {"Free up space and relaunch.","Free up space and relaunch.","Free up space and relaunch.","Free up space and relaunch.","Free up space and relaunch.","Free up space and relaunch."},
 {"Friendly name shown in the Cemu file browser.","Friendly name shown in the Cemu file browser.","Friendly name shown in the Cemu file browser.","Friendly name shown in the Cemu file browser.","Friendly name shown in the Cemu file browser.","Friendly name shown in the Cemu file browser."},
 {"Game deleted","Jeu supprimé","Spiel gelöscht","Juego eliminado","Gioco eliminato","Jogo eliminado"},
 {"Game folder","Dossier de jeux","Spieleordner","Carpeta de juegos","Cartella dei giochi","Pasta de jogos"},
 {"Game not found","Jeu introuvable","Spiel nicht gefunden","Juego no encontrado","Gioco non trovato","Jogo não encontrado"},
 {"Game settings cleared","Paramètres du jeu effacés","Spieleinstellungen gelöscht","Ajustes del juego borrados","Impostazioni gioco cancellate","Definições do jogo limpas"},
 {"Games path is not a folder","Games path is not a folder","Games path is not a folder","Games path is not a folder","Games path is not a folder","Games path is not a folder"},
 {"HOME shortcut installed","Raccourci HOME installé","HOME-Verknüpfung installiert","Acceso directo HOME instalado","Collegamento HOME installato","Atalho HOME instalado"},
 {"Imported Wii U account","Imported Wii U account","Imported Wii U account","Imported Wii U account","Imported Wii U account","Imported Wii U account"},
 {"Installed component","Installed component","Installed component","Installed component","Installed component","Installed component"},
 {"Installed component deleted","Installed component deleted","Installed component deleted","Installed component deleted","Installed component deleted","Installed component deleted"},
 {"Installing title...","Installing title...","Installing title...","Installing title...","Installing title...","Installing title..."},
 {"Invalid SMB server","Invalid SMB server","Invalid SMB server","Invalid SMB server","Invalid SMB server","Invalid SMB server"},
 {"Invalid SMB share","Invalid SMB share","Invalid SMB share","Invalid SMB share","Invalid SMB share","Invalid SMB share"},
 {"Invalid name","Nom invalide","Ungültiger Name","Nombre no válido","Nome non valido","Nome inválido"},
 {"LSFG disabled for this launch","LSFG disabled for this launch","LSFG disabled for this launch","LSFG disabled for this launch","LSFG disabled for this launch","LSFG disabled for this launch"},
 {"LSFG is not ready","LSFG is not ready","LSFG is not ready","LSFG is not ready","LSFG is not ready","LSFG is not ready"},
 {"Launch configuration failed","Launch configuration failed","Launch configuration failed","Launch configuration failed","Launch configuration failed","Launch configuration failed"},
 {"Leave blank for guest","Leave blank for guest","Leave blank for guest","Leave blank for guest","Leave blank for guest","Leave blank for guest"},
 {"Leave blank when no password is required","Leave blank when no password is required","Leave blank when no password is required","Leave blank when no password is required","Leave blank when no password is required","Leave blank when no password is required"},
 {"Loading available artwork...","Loading available artwork...","Loading available artwork...","Loading available artwork...","Loading available artwork...","Loading available artwork..."},
 {"Loading preview...","Loading preview...","Loading preview...","Loading preview...","Loading preview...","Loading preview..."},
 {"Location unavailable","Location unavailable","Location unavailable","Location unavailable","Location unavailable","Location unavailable"},
 {"Lossless.dll was not found at","Lossless.dll was not found at","Lossless.dll was not found at","Lossless.dll was not found at","Lossless.dll was not found at","Lossless.dll was not found at"},
 {"Lossless.dll was not found.","Lossless.dll was not found.","Lossless.dll was not found.","Lossless.dll was not found.","Lossless.dll was not found.","Lossless.dll was not found."},
 {"Maximum of 16 game folders","Maximum de 16 dossiers de jeux","Höchstens 16 Spieleordner","Máximo de 16 carpetas de juegos","Massimo 16 cartelle dei giochi","Máximo de 16 pastas de jogos"},
 {"Maximum of 24 pinned folders","Maximum de 24 dossiers épinglés","Höchstens 24 angeheftete Ordner","Máximo de 24 carpetas fijadas","Massimo 24 cartelle fissate","Máximo de 24 pastas afixadas"},
 {"Maximum of 8 SMB shares","Maximum de 8 partages SMB","Höchstens 8 SMB-Freigaben","Máximo de 8 recursos SMB","Massimo 8 condivisioni SMB","Máximo de 8 partilhas SMB"},
 {"Metadata missing or does not match the title ID","Métadonnées absentes ou incompatibles avec l’identifiant du titre","Metadaten fehlen oder passen nicht zur Titel-ID","Faltan metadatos o no coinciden con el ID del título","Metadati mancanti o non corrispondenti all’ID titolo","Metadados em falta ou não correspondem ao ID do título"},
 {"Metadata verified","Métadonnées vérifiées","Metadaten geprüft","Metadatos verificados","Metadati verificati","Metadados verificados"},
 {"Move complete","Déplacement terminé","Verschieben abgeschlossen","Movimiento completado","Spostamento completato","Movimentação concluída"},
 {"Move failed","Échec du déplacement","Verschieben fehlgeschlagen","Falló el movimiento","Spostamento non riuscito","Falha ao mover"},
 {"Move incomplete","Move incomplete","Move incomplete","Move incomplete","Move incomplete","Move incomplete"},
 {"Move queued","Move queued","Move queued","Move queued","Move queued","Move queued"},
 {"Names cannot contain /, \\\\, :, or control characters.","Names cannot contain /, \\\\, :, or control characters.","Names cannot contain /, \\\\, :, or control characters.","Names cannot contain /, \\\\, :, or control characters.","Names cannot contain /, \\\\, :, or control characters.","Names cannot contain /, \\\\, :, or control characters."},
 {"Nested folders are supported.","Nested folders are supported.","Nested folders are supported.","Nested folders are supported.","Nested folders are supported.","Nested folders are supported."},
 {"Network service","Network service","Network service","Network service","Network service","Network service"},
 {"No","Non","Nein","No","No","Não"},
 {"No controller is connected.","No controller is connected.","No controller is connected.","No controller is connected.","No controller is connected.","No controller is connected."},
 {"No files will be deleted.","No files will be deleted.","No files will be deleted.","No files will be deleted.","No files will be deleted.","No files will be deleted."},
 {"No games will be deleted.","No games will be deleted.","No games will be deleted.","No games will be deleted.","No games will be deleted.","No games will be deleted."},
 {"No icon found - add a SteamGridDB key or download a cover first","No icon found - add a SteamGridDB key or download a cover first","No icon found - add a SteamGridDB key or download a cover first","No icon found - add a SteamGridDB key or download a cover first","No icon found - add a SteamGridDB key or download a cover first","No icon found - add a SteamGridDB key or download a cover first"},
 {"No imported accounts","No imported accounts","No imported accounts","No imported accounts","No imported accounts","No imported accounts"},
 {"No metadata was removed.","No metadata was removed.","No metadata was removed.","No metadata was removed.","No metadata was removed.","No metadata was removed."},
 {"No packs - use Download latest packs","No packs - use Download latest packs","No packs - use Download latest packs","No packs - use Download latest packs","No packs - use Download latest packs","No packs - use Download latest packs"},
 {"No shader caches found","No shader caches found","No shader caches found","No shader caches found","No shader caches found","No shader caches found"},
 {"Not enough free space","Not enough free space","Not enough free space","Not enough free space","Not enough free space","Not enough free space"},
 {"Not found","Not found","Not found","Not found","Not found","Not found"},
 {"Offline / Nintendo / Pretendo","Offline / Nintendo / Pretendo","Offline / Nintendo / Pretendo","Offline / Nintendo / Pretendo","Offline / Nintendo / Pretendo","Offline / Nintendo / Pretendo"},
 {"Offline / Nintendo / Pretendo / Custom","Offline / Nintendo / Pretendo / Custom","Offline / Nintendo / Pretendo / Custom","Offline / Nintendo / Pretendo / Custom","Offline / Nintendo / Pretendo / Custom","Offline / Nintendo / Pretendo / Custom"},
 {"Online services require an imported account.dat.","Online services require an imported account.dat.","Online services require an imported account.dat.","Online services require an imported account.dat.","Online services require an imported account.dat.","Online services require an imported account.dat."},
 {"Online settings unavailable","Online settings unavailable","Online settings unavailable","Online settings unavailable","Online settings unavailable","Online settings unavailable"},
 {"Open Settings > Launcher > Check for Updates","Open Settings > Launcher > Check for Updates","Open Settings > Launcher > Check for Updates","Open Settings > Launcher > Check for Updates","Open Settings > Launcher > Check for Updates","Open Settings > Launcher > Check for Updates"},
 {"Password","Mot de passe","Passwort","Contraseña","Password","Palavra-passe"},
 {"Password for the SMB account. It is stored in launcher.ini.","Password for the SMB account. It is stored in launcher.ini.","Password for the SMB account. It is stored in launcher.ini.","Password for the SMB account. It is stored in launcher.ini.","Password for the SMB account. It is stored in launcher.ini.","Password for the SMB account. It is stored in launcher.ini."},
 {"Paste failed","Échec du collage","Einfügen fehlgeschlagen","Falló el pegado","Incolla non riuscito","Falha ao colar"},
 {"Pick an icon first","Pick an icon first","Pick an icon first","Pick an icon first","Pick an icon first","Pick an icon first"},
 {"Press the button or trigger to assign","Press the button or trigger to assign","Press the button or trigger to assign","Press the button or trigger to assign","Press the button or trigger to assign","Press the button or trigger to assign"},
 {"Press-to-bind","Press-to-bind","Press-to-bind","Press-to-bind","Press-to-bind","Press-to-bind"},
 {"Preview unavailable","Preview unavailable","Preview unavailable","Preview unavailable","Preview unavailable","Preview unavailable"},
 {"Reconnect its storage or update the game folders.","Reconnect its storage or update the game folders.","Reconnect its storage or update the game folders.","Reconnect its storage or update the game folders.","Reconnect its storage or update the game folders.","Reconnect its storage or update the game folders."},
 {"Release the current button","Release the current button","Release the current button","Release the current button","Release the current button","Release the current button"},
 {"Remove SMB share?","Remove SMB share?","Remove SMB share?","Remove SMB share?","Remove SMB share?","Remove SMB share?"},
 {"Remove game folder?","Remove game folder?","Remove game folder?","Remove game folder?","Remove game folder?","Remove game folder?"},
 {"Remove this folder manually to avoid deleting unrelated files:","Remove this folder manually to avoid deleting unrelated files:","Remove this folder manually to avoid deleting unrelated files:","Remove this folder manually to avoid deleting unrelated files:","Remove this folder manually to avoid deleting unrelated files:","Remove this folder manually to avoid deleting unrelated files:"},
 {"Rename collection","Renommer la collection","Sammlung umbenennen","Renombrar colección","Rinomina raccolta","Mudar nome da coleção"},
 {"Rename failed","Échec du renommage","Umbenennen fehlgeschlagen","Falló el cambio de nombre","Rinomina non riuscita","Falha ao mudar o nome"},
 {"Rename game","Renommer le jeu","Spiel umbenennen","Renombrar juego","Rinomina gioco","Mudar nome do jogo"},
 {"Renamed","Renommé","Umbenannt","Renombrado","Rinominato","Nome alterado"},
 {"Replace existing file?","Replace existing file?","Replace existing file?","Replace existing file?","Replace existing file?","Replace existing file?"},
 {"Review both locations before trying again.","Review both locations before trying again.","Review both locations before trying again.","Review both locations before trying again.","Review both locations before trying again.","Review both locations before trying again."},
 {"SMB connection failed","SMB connection failed","SMB connection failed","SMB connection failed","SMB connection failed","SMB connection failed"},
 {"SMB display name","SMB display name","SMB display name","SMB display name","SMB display name","SMB display name"},
 {"Safely eject USB drive?","Safely eject USB drive?","Safely eject USB drive?","Safely eject USB drive?","Safely eject USB drive?","Safely eject USB drive?"},
 {"Saved data was not touched.","Saved data was not touched.","Saved data was not touched.","Saved data was not touched.","Saved data was not touched.","Saved data was not touched."},
 {"Saved folders on this share will also be removed.","Saved folders on this share will also be removed.","Saved folders on this share will also be removed.","Saved folders on this share will also be removed.","Saved folders on this share will also be removed.","Saved folders on this share will also be removed."},
 {"Search games","Rechercher des jeux","Spiele suchen","Buscar juegos","Cerca giochi","Pesquisar jogos"},
 {"Searching SteamGridDB...","Searching SteamGridDB...","Searching SteamGridDB...","Searching SteamGridDB...","Searching SteamGridDB...","Searching SteamGridDB..."},
 {"Select a control","Select a control","Select a control","Select a control","Select a control","Select a control"},
 {"Select an account first","Select an account first","Select an account first","Select an account first","Select an account first","Select an account first"},
 {"Selecting a service does not create credentials or guarantee server access.","Selecting a service does not create credentials or guarantee server access.","Selecting a service does not create credentials or guarantee server access.","Selecting a service does not create credentials or guarantee server access.","Selecting a service does not create credentials or guarantee server access.","Selecting a service does not create credentials or guarantee server access."},
 {"Server or IP address","Server or IP address","Server or IP address","Server or IP address","Server or IP address","Server or IP address"},
 {"Service","Service","Service","Service","Service","Service"},
 {"Settings not saved","Paramètres non enregistrés","Einstellungen nicht gespeichert","Ajustes no guardados","Impostazioni non salvate","Definições não guardadas"},
 {"Shader caches cleared","Shader caches cleared","Shader caches cleared","Shader caches cleared","Shader caches cleared","Shader caches cleared"},
 {"Shared folder","Shared folder","Shared folder","Shared folder","Shared folder","Shared folder"},
 {"Shortcut failed","Échec du raccourci","Verknüpfung fehlgeschlagen","Falló el acceso directo","Collegamento non riuscito","Falha no atalho"},
 {"SteamGridDB API key rejected","SteamGridDB API key rejected","SteamGridDB API key rejected","SteamGridDB API key rejected","SteamGridDB API key rejected","SteamGridDB API key rejected"},
 {"The SD card may be full or write-protected.","The SD card may be full or write-protected.","The SD card may be full or write-protected.","The SD card may be full or write-protected.","The SD card may be full or write-protected.","The SD card may be full or write-protected."},
 {"The copied item is no longer available.","The copied item is no longer available.","The copied item is no longer available.","The copied item is no longer available.","The copied item is no longer available.","The copied item is no longer available."},
 {"The copy completed, but the original could not be removed completely.","The copy completed, but the original could not be removed completely.","The copy completed, but the original could not be removed completely.","The copy completed, but the original could not be removed completely.","The copy completed, but the original could not be removed completely.","The copy completed, but the original could not be removed completely."},
 {"The destination cannot be inside the source.","The destination cannot be inside the source.","The destination cannot be inside the source.","The destination cannot be inside the source.","The destination cannot be inside the source.","The destination cannot be inside the source."},
 {"The destination does not have enough available space.","The destination does not have enough available space.","The destination does not have enough available space.","The destination does not have enough available space.","The destination does not have enough available space.","The destination does not have enough available space."},
 {"The destination is not a regular file.","The destination is not a regular file.","The destination is not a regular file.","The destination is not a regular file.","The destination is not a regular file.","The destination is not a regular file."},
 {"The device may be disconnected.","The device may be disconnected.","The device may be disconnected.","The device may be disconnected.","The device may be disconnected.","The device may be disconnected."},
 {"The existing copy will be replaced.","The existing copy will be replaced.","The existing copy will be replaced.","The existing copy will be replaced.","The existing copy will be replaced.","The existing copy will be replaced."},
 {"The existing file will be replaced.","The existing file will be replaced.","The existing file will be replaced.","The existing file will be replaced.","The existing file will be replaced.","The existing file will be replaced."},
 {"The file transfer could not be completed.","The file transfer could not be completed.","The file transfer could not be completed.","The file transfer could not be completed.","The file transfer could not be completed.","The file transfer could not be completed."},
 {"The game file could not be removed.","The game file could not be removed.","The game file could not be removed.","The game file could not be removed.","The game file could not be removed.","The game file could not be removed."},
 {"The game will start automatically","The game will start automatically","The game will start automatically","The game will start automatically","The game will start automatically","The game will start automatically"},
 {"The installed launcher was left unchanged.","The installed launcher was left unchanged.","The installed launcher was left unchanged.","The installed launcher was left unchanged.","The installed launcher was left unchanged.","The installed launcher was left unchanged."},
 {"The launcher could not initialize its network connection.","The launcher could not initialize its network connection.","The launcher could not initialize its network connection.","The launcher could not initialize its network connection.","The launcher could not initialize its network connection.","The launcher could not initialize its network connection."},
 {"The previous settings were preserved.","The previous settings were preserved.","The previous settings were preserved.","The previous settings were preserved.","The previous settings were preserved.","The previous settings were preserved."},
 {"The selected game no longer exists.","The selected game no longer exists.","The selected game no longer exists.","The selected game no longer exists.","The selected game no longer exists.","The selected game no longer exists."},
 {"The selected path is not a regular game file.","The selected path is not a regular game file.","The selected path is not a regular game file.","The selected path is not a regular game file.","The selected path is not a regular game file.","The selected path is not a regular game file."},
 {"The service remains offline until that file is present.","The service remains offline until that file is present.","The service remains offline until that file is present.","The service remains offline until that file is present.","The service remains offline until that file is present.","The service remains offline until that file is present."},
 {"The shortcut's game is not in the current library.","The shortcut's game is not in the current library.","The shortcut's game is not in the current library.","The shortcut's game is not in the current library.","The shortcut's game is not in the current library.","The shortcut's game is not in the current library."},
 {"This permanently deletes the game file from","This permanently deletes the game file from","This permanently deletes the game file from","This permanently deletes the game file from","This permanently deletes the game file from","This permanently deletes the game file from"},
 {"Touch anywhere to close","Touch anywhere to close","Touch anywhere to close","Touch anywhere to close","Touch anywhere to close","Touch anywhere to close"},
 {"Touch left to clear       Touch right to cancel","Touch left to clear       Touch right to cancel","Touch left to clear       Touch right to cancel","Touch left to clear       Touch right to cancel","Touch left to clear       Touch right to cancel","Touch left to clear       Touch right to cancel"},
 {"Transfer cancelled","Transfert annulé","Übertragung abgebrochen","Transferencia cancelada","Trasferimento annullato","Transferência cancelada"},
 {"Transfer complete","Transfert terminé","Übertragung abgeschlossen","Transferencia completada","Trasferimento completato","Transferência concluída"},
 {"Transfer failed","Échec du transfert","Übertragung fehlgeschlagen","Falló la transferencia","Trasferimento non riuscito","Falha na transferência"},
 {"USB drive can now be removed","USB drive can now be removed","USB drive can now be removed","USB drive can now be removed","USB drive can now be removed","USB drive can now be removed"},
 {"USB eject failed","USB eject failed","USB eject failed","USB eject failed","USB eject failed","USB eject failed"},
 {"Unknown error","Unknown error","Unknown error","Unknown error","Unknown error","Unknown error"},
 {"Unnamed Wii U title","Unnamed Wii U title","Unnamed Wii U title","Unnamed Wii U title","Unnamed Wii U title","Unnamed Wii U title"},
 {"Update check unavailable","Update check unavailable","Update check unavailable","Update check unavailable","Update check unavailable","Update check unavailable"},
 {"Update recovery failed","Update recovery failed","Update recovery failed","Update recovery failed","Update recovery failed","Update recovery failed"},
 {"Update the saved API key in Launcher settings.","Update the saved API key in Launcher settings.","Update the saved API key in Launcher settings.","Update the saved API key in Launcher settings.","Update the saved API key in Launcher settings.","Update the saved API key in Launcher settings."},
 {"Username","Nom d’utilisateur","Benutzername","Nombre de usuario","Nome utente","Nome de utilizador"},
 {"Usually optional on a home network.","Usually optional on a home network.","Usually optional on a home network.","Usually optional on a home network.","Usually optional on a home network.","Usually optional on a home network."},
 {"WUX games cannot be decrypted until a valid key is added.","WUX games cannot be decrypted until a valid key is added.","WUX games cannot be decrypted until a valid key is added.","WUX games cannot be decrypted until a valid key is added.","WUX games cannot be decrypted until a valid key is added.","WUX games cannot be decrypted until a valid key is added."},
 {"Waiting for the game drive...","Waiting for the game drive...","Waiting for the game drive...","Waiting for the game drive...","Waiting for the game drive...","Waiting for the game drive..."},
 {"Workgroup","Groupe de travail","Arbeitsgruppe","Grupo de trabajo","Gruppo di lavoro","Grupo de trabalho"},
 {"Yes","Oui","Ja","Sí","Sì","Sim"},
 {"[ Add SMB share ]","[ Add SMB share ]","[ Add SMB share ]","[ Add SMB share ]","[ Add SMB share ]","[ Add SMB share ]"},
 {"browse / download","browse / download","browse / download","browse / download","browse / download","browse / download"},
 {"games / files / network","games / files / network","games / files / network","games / files / network","games / files / network","games / files / network"},
 {"settings.xml could not be read safely.","settings.xml could not be read safely.","settings.xml could not be read safely.","settings.xml could not be read safely.","settings.xml could not be read safely.","settings.xml could not be read safely."},
 {"settings.xml could not be updated safely.","settings.xml could not be updated safely.","settings.xml could not be updated safely.","settings.xml could not be updated safely.","settings.xml could not be updated safely.","settings.xml could not be updated safely."},
 {"the SD card. This cannot be undone.","the SD card. This cannot be undone.","the SD card. This cannot be undone.","the SD card. This cannot be undone.","the SD card. This cannot be undone.","the SD card. This cannot be undone."},
 {"then enable LSFG again.","then enable LSFG again.","then enable LSFG again.","then enable LSFG again.","then enable LSFG again.","then enable LSFG again."},
 {"Base games, updates, and DLC","Jeux de base, mises à jour et DLC","Basisspiele, Updates und DLC","Juegos base, actualizaciones y DLC","Giochi base, aggiornamenti e DLC","Jogos base, atualizações e DLC"},
 {"Current: ","Actuel : ","Aktuell: ","Actual: ","Attuale: ","Atual: "},
 {"Options","Options","Optionen","Opciones","Opzioni","Opções"},
 {"Setting info","Informations sur le paramètre","Einstellungsinfo","Información del ajuste","Informazioni impostazione","Informações da definição"},
 {"Installed to games folder","Installé dans le dossier des jeux","Im Spieleordner installiert","Instalado en la carpeta de juegos","Installato nella cartella dei giochi","Instalado na pasta de jogos"},
 {"Copy failed (SD space?)","Échec de la copie (espace SD ?)","Kopieren fehlgeschlagen (SD-Speicher?)","Falló la copia (¿espacio SD?)","Copia non riuscita (spazio SD?)","Falha ao copiar (espaço no SD?)"},
 {"Installed game content","Contenu du jeu installé","Installierte Spielinhalte","Contenido del juego instalado","Contenuti gioco installati","Conteúdo do jogo instalado"},
 {"No installed content for this game","Aucun contenu installé pour ce jeu","Keine installierten Inhalte für dieses Spiel","No hay contenido instalado para este juego","Nessun contenuto installato per questo gioco","Nenhum conteúdo instalado para este jogo"},
 {"No installed content found","Aucun contenu installé trouvé","Keine installierten Inhalte gefunden","No se encontró contenido instalado","Nessun contenuto installato trovato","Nenhum conteúdo instalado encontrado"},
};

void skipJsonWhitespace(const std::string& json,size_t& position)
{
 while(position<json.size()&&(json[position]==' '||json[position]=='\t'||json[position]=='\r'||json[position]=='\n')) position++;
}

int hexDigit(char value)
{
 if(value>='0'&&value<='9') return value-'0';
 if(value>='a'&&value<='f') return value-'a'+10;
 if(value>='A'&&value<='F') return value-'A'+10;
 return -1;
}

bool appendUtf8(std::string& output,uint32_t codepoint)
{
 if(codepoint<=0x7f) output.push_back(static_cast<char>(codepoint));
 else if(codepoint<=0x7ff){output.push_back(static_cast<char>(0xc0|(codepoint>>6)));output.push_back(static_cast<char>(0x80|(codepoint&0x3f)));}
 else if(codepoint<=0xffff){
  if(codepoint>=0xd800&&codepoint<=0xdfff) return false;
  output.push_back(static_cast<char>(0xe0|(codepoint>>12)));output.push_back(static_cast<char>(0x80|((codepoint>>6)&0x3f)));output.push_back(static_cast<char>(0x80|(codepoint&0x3f)));
 }
 else if(codepoint<=0x10ffff){output.push_back(static_cast<char>(0xf0|(codepoint>>18)));output.push_back(static_cast<char>(0x80|((codepoint>>12)&0x3f)));output.push_back(static_cast<char>(0x80|((codepoint>>6)&0x3f)));output.push_back(static_cast<char>(0x80|(codepoint&0x3f)));}
 else return false;
 return true;
}

bool parseJsonString(const std::string& json,size_t& position,std::string& output)
{
 if(position>=json.size()||json[position]!='"') return false;
 output.clear(); position++;
 while(position<json.size()){
  const unsigned char value=static_cast<unsigned char>(json[position++]);
  if(value=='"') return true;
  if(value<0x20) return false;
  if(value!='\\'){output.push_back(static_cast<char>(value));continue;}
  if(position>=json.size()) return false;
  const char escaped=json[position++];
  switch(escaped){
   case '"': output.push_back('"'); break;
   case '\\': output.push_back('\\'); break;
   case '/': output.push_back('/'); break;
   case 'b': output.push_back('\b'); break;
   case 'f': output.push_back('\f'); break;
   case 'n': output.push_back('\n'); break;
   case 'r': output.push_back('\r'); break;
   case 't': output.push_back('\t'); break;
   case 'u': {
    if(position+4>json.size()) return false;
    uint32_t codepoint=0;
    for(int digit=0;digit<4;digit++){const int part=hexDigit(json[position++]);if(part<0)return false;codepoint=(codepoint<<4)|static_cast<uint32_t>(part);}
    if(codepoint>=0xd800&&codepoint<=0xdbff){
     if(position+6>json.size()||json[position]!='\\'||json[position+1]!='u') return false;
     position+=2; uint32_t low=0;
     for(int digit=0;digit<4;digit++){const int part=hexDigit(json[position++]);if(part<0)return false;low=(low<<4)|static_cast<uint32_t>(part);}
     if(low<0xdc00||low>0xdfff) return false;
     codepoint=0x10000+((codepoint-0xd800)<<10)+(low-0xdc00);
    }
    if(!appendUtf8(output,codepoint)) return false;
    break;
   }
   default: return false;
  }
 }
 return false;
}

bool readTranslationFile(const char* path,std::string& contents)
{
 FILE* file=fopen(path,"rb");
 if(!file) return false;
 bool valid=fseek(file,0,SEEK_END)==0;
 const long length=valid?ftell(file):-1;
 valid=valid&&length>=0&&length<=1024*1024&&fseek(file,0,SEEK_SET)==0;
 if(valid){contents.assign(static_cast<size_t>(length),'\0');valid=contents.empty()||fread(contents.data(),1,contents.size(),file)==contents.size();}
 if(fclose(file)!=0) valid=false;
 if(!valid) contents.clear();
 return valid;
}

bool loadJsonTranslations(const char* path)
{
 std::string json;
 if(!readTranslationFile(path,json)) return false;
 size_t position=json.size()>=3&&static_cast<unsigned char>(json[0])==0xef&&static_cast<unsigned char>(json[1])==0xbb&&static_cast<unsigned char>(json[2])==0xbf?3:0;
 skipJsonWhitespace(json,position);
 if(position>=json.size()||json[position++]!='{') return false;
 std::unordered_map<std::string,std::string> translations;
 for(;;){
  skipJsonWhitespace(json,position);
  if(position<json.size()&&json[position]=='}'){position++;break;}
  std::string key,value;
  if(!parseJsonString(json,position,key)) return false;
  skipJsonWhitespace(json,position);
  if(position>=json.size()||json[position++]!=':') return false;
  skipJsonWhitespace(json,position);
  if(!parseJsonString(json,position,value)||!translations.emplace(std::move(key),std::move(value)).second) return false;
  skipJsonWhitespace(json,position);
  if(position>=json.size()) return false;
  if(json[position]=='}'){position++;break;}
  if(json[position++]!=',') return false;
 }
 skipJsonWhitespace(json,position);
 if(position!=json.size()) return false;
 for(const Entry& entry:ENTRIES) if(translations.find(entry.en)==translations.end()) return false;
 s_translations=std::move(translations);
 return true;
}

std::string systemLanguage()
{
 if(R_FAILED(setInitialize())) return "en";
 u64 code=0; SetLanguage language=SetLanguage_ENUS;
 Result result=setGetSystemLanguage(&code);
 if(R_SUCCEEDED(result)) result=setMakeLanguage(code,&language);
 setExit();
 if(R_FAILED(result)) return "en";
 switch(language){case SetLanguage_FR:case SetLanguage_FRCA:return "fr";case SetLanguage_DE:return "de";case SetLanguage_ES:case SetLanguage_ES419:return "es";case SetLanguage_IT:return "it";case SetLanguage_PT:case SetLanguage_PTBR:return "pt";case SetLanguage_ZHCN:case SetLanguage_ZHHANS:return "zh-CN";case SetLanguage_ZHTW:case SetLanguage_ZHHANT:return "zh-TW";default:return "en";}
}
}

void Initialize(std::string_view preference)
{
 s_preference=preference.empty()?"system":std::string(preference); s_language=s_preference=="system"?systemLanguage():s_preference;
 if(FindLanguage(s_language)<1) s_language="en";
 s_translations.clear();
 if(s_language=="zh-CN"){loadJsonTranslations("romfs:/localization/cemu_zh-CN.json");return;}
 if(s_language=="zh-TW"){loadJsonTranslations("romfs:/localization/cemu_zh-TW.json");return;}
 const int column=s_language=="fr"?1:s_language=="de"?2:s_language=="es"?3:s_language=="it"?4:s_language=="pt"?5:0;
 if(!column) return;
 for(const Entry& entry:ENTRIES){const char* values[]={entry.en,entry.fr,entry.de,entry.es,entry.it,entry.pt};s_translations.emplace(entry.en,values[column]);}
}
std::string_view Translate(std::string_view source){const auto it=s_translations.find(std::string(source));return it==s_translations.end()?source:std::string_view(it->second);}
std::string_view Preference(){return s_preference;}
std::string_view ActiveLanguage(){return s_language;}
std::string DisplayName(){for(const auto& language:s_languages)if(language.code==s_preference)return language.name;return "English";}
const std::vector<Language>& Languages(){return s_languages;}
int FindLanguage(std::string_view code){for(size_t i=0;i<s_languages.size();i++)if(code==s_languages[i].code)return static_cast<int>(i);return -1;}
}
