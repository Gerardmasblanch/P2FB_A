import com.fazecast.jSerialComm.SerialPort;
import com.fazecast.jSerialComm.SerialPortDataListener;
import com.fazecast.jSerialComm.SerialPortEvent;

import javax.swing.*;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

public class Controller {

    private UI ui;
    private SerialPort serialPort;
    private boolean connected = false;
    private JButton[][] gridAnimals;
    private JButton[][] botonesInferiores;
    private JButton[] botonesConsum;
    private boolean inAreaConsum;
    private JButton init;
    private int filaActual = 0;
    private int colActual = 0;
    private List<Animal> animales = new ArrayList<>();
    private boolean inRebelion = false;
    private int lastAnimal_fila;
    private int lastAnimal_columna;
    private ArrayList<Byte> buffer;
    private boolean hayAnimales = false;
    private boolean initDone;
    class Animal {
        String nombre;
        String estado;
        int num;

        public Animal(String nombre, String estado, int num) {
            this.nombre = nombre;
            this.estado = estado;
            this.num = num;
        }
    }

    public Controller(UI ui) {
        this.ui = ui;
        gridAnimals = ui.getBotonesFotos();
        botonesInferiores = ui.getBotonesInferiores();
        init = ui.getBotonSuperior();
        botonesConsum = ui.getBotonesConsum();
        inAreaConsum = false;
        actualizarSeleccion();
    }

    // =============================
    // SERIAL
    // =============================

    public void refreshPuertos() {
        ui.limpiarPuertos();
        ui.appendConsola("Refreshing ports...\n");
        SerialPort[] ports = SerialPort.getCommPorts();
        for (SerialPort port : ports) {
            ui.addPuerto(port.getSystemPortName());
        }
        if (ports.length == 0){
            ui.appendConsola("\nNo serial ports available.\n");
        }
    }

    public void conectar(String puerto, int baudrate) {
        serialPort = SerialPort.getCommPort(puerto);
        serialPort.setBaudRate(baudrate);

        if (serialPort.openPort()) {
            connected = true;
            ui.setConnected(true);
            escucharSerial();
        } else {
            System.out.println("Port not available\n");
            ui.appendConsola("\nPort not available :(\n");
        }
    }

    public void desconectar() {
        if (serialPort != null && serialPort.isOpen()) {
            serialPort.closePort();
        }
        connected = false;
        ui.setConnected(false);
    }

    public void checkConnectEnabled() {
        boolean puertoOK = ui.getPuertoSeleccionado() != null && !ui.getPuertoSeleccionado().equals("Select Port");
        boolean baudOK = ui.getBaudSeleccionado() != null && !ui.getBaudSeleccionado().equals("Select Baud Rate");

        ui.enableConnect(!connected && puertoOK && baudOK);
    }

    // =============================
    // LECTURA SERIAL
    // =============================

    private void escucharSerial() {
        buffer = new ArrayList<>();
        serialPort.addDataListener(new SerialPortDataListener() {
            @Override
            public int getListeningEvents() {
                return SerialPort.LISTENING_EVENT_DATA_AVAILABLE;
            }
            @Override
            public void serialEvent(SerialPortEvent event) {
                if (event.getEventType() != SerialPort.LISTENING_EVENT_DATA_AVAILABLE) return;
                byte[] newData = new byte[serialPort.bytesAvailable()];
                serialPort.readBytes(newData, newData.length);
                for (byte b : newData) {
                    buffer.add( b );
                    if((char)b == '\n'){
                        interpretar(getBufferAsArray());
                        buffer = new ArrayList<>();
                    }
                }


            }
        });
    }

    private byte[] getBufferAsArray() {
        byte[] byteArray = new byte[buffer.size()];
        for (int i = 0; i < buffer.size(); i++) {
            byteArray[i] = buffer.get(i);
        }
        return byteArray;
    }
    private void interpretar(byte[] newData) {
        String rawText = new String(newData, StandardCharsets.ISO_8859_1).trim();
        final String receivedText = rawText.replaceAll("[^\\x20-\\x7E]", "");
        ui.appendConsola(receivedText);
        switch (receivedText) {
            case FarmConstants.CMD_UP -> moverArriba();
            case FarmConstants.CMD_DOWN -> moverAbajo();
            case FarmConstants.CMD_LEFT -> moverIzquierda();
            case FarmConstants.CMD_RIGHT -> moverDerecha();
            case FarmConstants.CMD_SELECT -> seleccionar();
            default -> comandes(receivedText);
        }
    }

    private void comandes(String cmd){
        if (cmd.equals(FarmConstants.CMD_FINISH)){             //printar els animals
            mostrarAnimales();
            gridAnimals = ui.getBotonesFotos();
            hayAnimales = true;
        } else if (cmd.startsWith(FarmConstants.CMD_DATA_PRODUCTS)){
            leerProductos(cmd);
        } else if (cmd.equals(FarmConstants.CMD_SLEEP_SUCCESSFUL)){
            //marcar al animal (lo tendre que tener guardado) como activo
            int index = (int) gridAnimals[lastAnimal_fila-1][lastAnimal_columna].getClientProperty("animalId");
            Animal a = animales.get(index);
            a.estado = "AWAKE";
            animales.set(index, a);
            mostrarAnimales();
            gridAnimals = ui.getBotonesFotos();
            actualizarSeleccion();
        } else if (cmd.equals(FarmConstants.CMD_SLEEP_UNSUCCESSFUL)){
            //poner al animal (lo tendre que tener guardado) como dormido, NO CAMBIAR NADA
        } else if (cmd.startsWith(FarmConstants.CMD_DATA_ANIMAL)){
            //ir metiendo en un array o algo con el estate hasta el finish
            leerAnimal(cmd);
        }  else {
            ui.appendConsola("Unknown command: " + cmd + "\n");
        }
    }

    private void leerAnimal(String cmd){
        try{
            String data = cmd.replace(FarmConstants.CMD_DATA_ANIMAL, "");
            String[] partes = data.split("\\$");
            String nombre = partes[0];
            int num = Integer.parseInt(partes[1]);
            String estado = partes[2];
            animales.add(new Animal(nombre, estado, num));
        }catch(Exception e){
            ui.appendConsola("\nError reading animals\n");
        }
    }

    private void mostrarAnimales(){
        int[] numeros = new int[animales.size()];
        String[] estados = new String[animales.size()];
        boolean[] activos = new boolean[animales.size()];
        for(int i=0;i<animales.size();i++){
            estados[i] = animales.get(i).nombre;
            activos[i] = animales.get(i).estado.equals("AWAKE");
            numeros[i] = animales.get(i).num;
        }
        if (estados.length > 18) {
            ui.resetPanelAnimalesDos(estados, activos, numeros);
        } else{
            ui.resetPanelAnimales(estados, activos, numeros);
        }
        gridAnimals = ui.getBotonesFotos();
    }

    private void leerProductos(String cmd){
        try{
            //FarmConstants.CMD_DATA_PRODUCTS + “NumLlet$NumPernil$NumOus$NumPinzells\r\n”
            String data = cmd.replace(FarmConstants.CMD_DATA_PRODUCTS,"");
            String[] valores = data.split("\\$");
            int[] productos = new int[4];
            for(int i=0;i<4;i++){
                productos[i] = Integer.parseInt(valores[i]);
            }
            ui.mostrarGrafico(productos);
        }catch(Exception e){
            ui.appendConsola("\nError reading products\n");
        }

    }

    public void moverArriba() {
        if (!initDone){
            return;
        }
        if (inAreaConsum){
            if (filaActual > 0){
                filaActual--;
            }
        } else {
            if (filaActual == 1) {
                return;
            }
            if (filaActual > 0) {
                if (hayAnimales) {
                    int max_cols = gridAnimals[0].length;

                    if (filaActual == 4) {
                        if (!((max_cols < 7 && colActual == 0) || (max_cols >= 7 && (colActual == 0 || colActual == 1)))) {
                            return;
                        }
                    }
                } else {
                    if (filaActual == 4){
                        return;
                    }
                }
                if (filaActual >= 1 && filaActual <= 3 && gridAnimals[filaActual-1-1][colActual] ==null){
                    // || (filaActual == 4 && colActual ==1)
                    return;
                }
                filaActual--;
                if (filaActual == 3 && colActual == 0 && (animales.size() == 4 || animales.size() == 2)){
                    filaActual--;
                }
            }
        }

        actualizarSeleccion();
    }
    public void moverAbajo() {
        if (!initDone){
            return;
        }
        if (inAreaConsum){
            if (filaActual < 4){
                filaActual++;
            }
        } else {
            if (filaActual == 3 && colActual != 0 && colActual != 1) {
                return;
            }
            if (filaActual < 6) {
                if (filaActual >= 1 && filaActual <= 3) {
                    int filaSiguiente = filaActual+1; // gridAnimals usa 0..2

                    if (filaSiguiente <= gridAnimals.length) {
                        if (gridAnimals[filaActual][colActual] != null) {
                            filaActual++;
                            actualizarSeleccion();
                        } else {
                            if (filaSiguiente == 3 && colActual == 0 && (animales.size() == 4 || animales.size() == 2)){
                                filaActual++;
                                filaActual++;
                                actualizarSeleccion();
                            }
                        }
                        return;
                    }
                    int max_cols = gridAnimals[filaActual-1].length;
                    if (filaSiguiente == 4 && ((max_cols < 7 && colActual == 0) || (max_cols >= 7 && (colActual == 0 || colActual == 1)))) {
                        // permitido
                    } else {
                        return;
                    }
                }
                if (filaActual == 5 && colActual == 1){
                    return;
                }
                filaActual++;
                if (filaActual == 3 && colActual == 0 && animales.size() == 4){
                    filaActual++;
                }
            }
        }

        actualizarSeleccion();
    }
    public void moverIzquierda() {
        if (!initDone){
            return;
        }
        if (!inAreaConsum) {

            if (colActual > 0) {
                if (filaActual >= 1 && filaActual <= 3 && gridAnimals[filaActual-1][colActual-1] ==null){
                    return;
                }
                colActual--;
            }

            actualizarSeleccion();
        }
    }
    public void moverDerecha() {
        if (!initDone){
        return;
        }
        if (!inAreaConsum) {
            int maxCols;

            if (filaActual >= 1 && filaActual <= 3) {
                maxCols = gridAnimals[filaActual - 1].length;
            } else {
                maxCols = 2;
            }

            if (colActual < maxCols - 1) {
                if (filaActual >= 1 && filaActual <= 3 && gridAnimals[filaActual-1][colActual+1] ==null || filaActual == 6){
                    return;
                }
                colActual++;
            }

            actualizarSeleccion();
        }
    }
    public void seleccionar() {
        //en algun momento enseñar por la terminal
        if (!inAreaConsum) {
             if (filaActual >= 1 && filaActual <= 3) {
                int index = (int) gridAnimals[filaActual-1][colActual].getClientProperty("animalId");
                Animal a = animales.get(index);
                if (a.estado.equals("SLEEP")) {
                    gridAnimals[filaActual - 1][colActual].doClick();
                    lastAnimal_columna = colActual;
                    lastAnimal_fila = filaActual;
                    enviarSerialMensaje(FarmConstants.CMD_SLEEP + a.nombre+ "$"+ gridAnimals[filaActual - 1][colActual].getClientProperty("num_animal_especie"));
                }
            } else if (filaActual == 0 && colActual == 0){
                //initialize
                ui.mostrarDialogoInicializar();
            } else if (filaActual == 4 && colActual == 0){
                //enviar un get animals
                enviarSerialMensaje(FarmConstants.CMD_GET_ANIMALS);
                animales.clear();
                hayAnimales = false;
            } else if (filaActual == 4 && colActual == 1){
                //enviar reset
                enviarSerialMensaje(FarmConstants.CMD_RESET);
                ui.clearAnimals();
                filaActual = 0;
                colActual = 0;
                actualizarSeleccion();
            } else if (filaActual == 5 && colActual == 0){
                //enviar get products
                enviarSerialMensaje(FarmConstants.CMD_GET_PRODUCTS);
            } else if (filaActual == 5 && colActual == 1){
                //canviar pantalla, mes marcar el boto que toca
                ui.mostrarConsum();
                ui.ponerBorde(botonesConsum[0]);
                filaActual = 0;
                inAreaConsum = true;
            } else if (filaActual ==6 ){
                if (!inRebelion) {
                    enviarSerialMensaje(FarmConstants.CMD_START_REBELLION);
                    inRebelion = true;
                    ui.cambiarTexto(botonesInferiores[filaActual][0], "Stop Rebellion");
                } else{
                    enviarSerialMensaje(FarmConstants.CMD_STOP_REBELLION);
                    ui.cambiarTexto(botonesInferiores[filaActual][0], "Start Rebellion");

                    inRebelion = false;
                }
            }
        } else {
            //check donde estamos para enviar el mensaje que toca
            switch(filaActual){
                case 0:
                    enviarSerialMensaje(FarmConstants.CMD_CONSUME + "0");
                    break;
                case 1:
                    enviarSerialMensaje(FarmConstants.CMD_CONSUME +"1");
                    break;
                case 2:
                    enviarSerialMensaje(FarmConstants.CMD_CONSUME+"2");
                    break;
                case 3:
                    enviarSerialMensaje(FarmConstants.CMD_CONSUME+"3");
                    break;
                case 4:
                    ui.mostrarPantallaPrincipal();
                    filaActual = 5;
                    colActual = 1;
                    inAreaConsum = false;
                    actualizarSeleccion();
                    break;
            }
        }
    }

    private void actualizarSeleccion() {
        ui.limpiarTodosLosBordes();
        if (inAreaConsum){
            ui.ponerBorde(botonesConsum[filaActual]);
        } else {
            if (filaActual == 0) {
                ui.ponerBorde(init);
            } else if (filaActual >= 1 && filaActual <= 3) {
                if (gridAnimals[filaActual - 1][colActual] == null){
                    if (gridAnimals[filaActual - 2][colActual] == null){
                        ui.ponerBorde(gridAnimals[filaActual - 1-2][colActual]);
                    }else{
                        ui.ponerBorde(gridAnimals[filaActual - 1-1][colActual]);
                    }
                }else {
                    ui.ponerBorde(gridAnimals[filaActual - 1][colActual]);
                }
            } else {
                ui.ponerBorde(botonesInferiores[filaActual][colActual]);
            }
        }
    }

    public void inicializar(String nombre, int t1, int t2, int t3, int t4) {
        String texto = FarmConstants.CMD_INITIALIZE + nombre + "$" + t1 + "$" + t2 + "$" + t3 + "$" + t4;
        System.out.println("Farm: " + nombre);
        System.out.println("Times: " + t1 + ", " + t2 + ", " + t3 + ", " + t4);
        int i = enviarSerialMensaje(texto);
        if (i != -1){
            filaActual = 4;
            colActual = 0;
            actualizarSeleccion();
            initDone = true;
        }
    }

    public int enviarSerialMensaje(String mensaje) {
        if (serialPort != null && serialPort.isOpen()) {
            String datos = (mensaje + "\r\n");
            serialPort.writeBytes(datos.getBytes(StandardCharsets.US_ASCII), datos.length());
            ui.appendConsola("Sent: " + mensaje + "\n");
            System.out.println("Sent: " + mensaje + "\n");
            return 1;
        } else {
            ui.appendConsola("\nPort is not open");
            return -1;
        }
    }

}