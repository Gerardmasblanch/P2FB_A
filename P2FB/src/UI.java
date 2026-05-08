import javax.swing.*;
import javax.swing.text.DefaultCaret;
import java.awt.*;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import javax.swing.JButton;


public class UI extends JFrame {

    private JComboBox<String> comboPuertos;
    private JComboBox<String> comboBaud;
    private JButton btnRefresh;
    private JButton btnConnect;
    private JButton btnDisconnect;
    private JPanel panelFotos;
    private Controller controller;
    private JButton btnInitialize;
    private JButton[][] botonesAnimales;
    private JButton[][] botonesInferiores = new JButton[7][];
    private JPanel panelConsum;
    private CardLayout cardLayout;
    private JPanel mainPanel;
    private JTextArea consola;
    private JTextArea consolaConsum;
    private JButton[] botonesConsum;

    public void setController(Controller controller) {
        this.controller = controller;
    }

    private JButton crearBotonImagen(String rutaImagen, int ancho, int alto, String texto) {

        JButton boton = new JButton(texto) {

            private Image imagenOriginal = new ImageIcon(rutaImagen).getImage();

            @Override
            protected void paintComponent(Graphics g) {
                super.paintComponent(g);
                g.drawImage(imagenOriginal, 0, 0, getWidth(), getHeight(), this);
                super.paintComponent(g);
            }
        };

        boton.setPreferredSize(new Dimension(ancho, alto));
        boton.setHorizontalTextPosition(SwingConstants.CENTER);
        boton.setVerticalTextPosition(SwingConstants.CENTER);

        boton.setBorderPainted(false);
        boton.setContentAreaFilled(false);
        boton.setFocusPainted(false);
        boton.setOpaque(false);
        boton.setForeground(Color.BLACK);
        boton.setHorizontalTextPosition(SwingConstants.CENTER);
        boton.setVerticalTextPosition(SwingConstants.CENTER);

        boton.setFont(new Font("Arial", Font.BOLD, 10));

        return boton;
    }


    private JPanel crearPanelFotos() {

        panelFotos = new JPanel(new GridLayout(3, 0, 10, 10)); // 3 filas, columnas automáticas
        panelFotos.setOpaque(false);

        botonesAnimales = new JButton[3][8];

        int f = 0;
        int c = 0;
        // Ejemplo de creación de botones
        for (int i = 1; i <= 24; i++) {
            JButton fotoBtn = crearBotonImagen("assets/cerdo_dorm.png", 100, 80, String.valueOf(i));
            botonesAnimales[f][c] = fotoBtn;
            panelFotos.add(fotoBtn);
            fotoBtn.setVisible(false);
            c++;
            if (i == 8 || i == 16 || i == 24){
                f++;
                c = 0;
            }

        }

        return panelFotos;
    }

    private JPanel crearConfigs() {

        JPanel panelSuperior = new JPanel(new FlowLayout(FlowLayout.LEFT, 10, 5));
        panelSuperior.setOpaque(false);

        comboPuertos = new JComboBox<>();
        comboPuertos.addItem("Select Port");
        comboPuertos.setSelectedIndex(0);
        comboPuertos.setPreferredSize(new Dimension(140, 25));

        comboBaud = new JComboBox<>(new String[]{
                "Select Baud Rate",
                "50", "75", "110", "134", "150", "200", "300", "600", "1200", "1800", "2400",
                "4800", "9600", "19200", "28800", "38400", "57600", "76800", "115200",
                "230400", "460800", "576000", "921600"
        });
        comboBaud.setSelectedIndex(0);
        comboBaud.setPreferredSize(new Dimension(140, 25));

        btnRefresh = new JButton("Refresh");
        btnConnect = new JButton("Connect");
        btnDisconnect = new JButton("Disconnect");
        btnInitialize = crearBotonImagen("assets/bala_paja.jpg", 120, 30, "Initialize");

        btnDisconnect.setEnabled(false);
        btnConnect.setEnabled(false);

        panelSuperior.add(comboPuertos);
        panelSuperior.add(comboBaud);
        panelSuperior.add(btnRefresh);
        panelSuperior.add(btnConnect);
        panelSuperior.add(btnDisconnect);
        panelSuperior.add(btnInitialize);
        configurarEventosSerial();

        return panelSuperior;
    }


    private void configurarEventosSerial() {

        comboPuertos.addActionListener(e -> controller.checkConnectEnabled());
        comboBaud.addActionListener(e -> controller.checkConnectEnabled());

        btnRefresh.addActionListener(e -> controller.refreshPuertos());

        btnConnect.addActionListener(e -> {
            String puerto = (String) comboPuertos.getSelectedItem();
            int baud = Integer.parseInt((String) comboBaud.getSelectedItem());
            controller.conectar(puerto, baud);
        });

        btnDisconnect.addActionListener(e -> controller.desconectar());
    }

    public JButton[][] getBotonesFotos() {
        return botonesAnimales;
    }

    public JButton[][] getBotonesInferiores() {
        return botonesInferiores;
    }

    public JButton[] getBotonesConsum(){
        return botonesConsum;
    }

    public JButton getBotonSuperior() {
        return btnInitialize;
    }
    public void limpiarTodosLosBordes() {
        for (JButton[] fila : botonesAnimales) {
            for (JButton b : fila) {
                if (b != null) {
                    quitarBorde(b);
                }
            }
        }

        for (JButton[] fila : botonesInferiores) {
            if (fila != null) {
                for (JButton b : fila) {
                    quitarBordeInit(b);
                }
            }
        }
        for (JButton b : botonesConsum){
            quitarBordeInit(b);
        }
        quitarBordeInit(btnInitialize);
    }

    public UI() {
        setTitle("P2FB");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setSize(800, 600);
        setLocationRelativeTo(null); // Centrar ventana

        // Panel de fondo con la imagen
        JPanel fondoPanel = new JPanel() {
            private Image background = new ImageIcon("assets/fondo.png").getImage();
            @Override
            protected void paintComponent(Graphics g) {
                super.paintComponent(g);
                g.drawImage(background, 0, 0, getWidth(), getHeight(), this);
            }
        };
        fondoPanel.setLayout(new BorderLayout());
        cardLayout = new CardLayout();
        mainPanel = new JPanel(cardLayout);
        setContentPane(mainPanel);

        JPanel panelSuperior = crearConfigs(); //Crear panel de configs

        // add arriba
        fondoPanel.add(panelSuperior, BorderLayout.NORTH);

        // Panel transparente encima para consola y botones
        JPanel overlayPanel = new JPanel();
        overlayPanel.setOpaque(false);
        overlayPanel.setLayout(new BorderLayout());
        overlayPanel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10)); // margen

        // Consola
        JTextArea consola = new JTextArea() {
            private Color fondo = new Color(0,0,0,150);

            @Override
            protected void paintComponent(Graphics g) {
                // Pintar fondo semi-transparente
                g.setColor(fondo);
                g.fillRect(0,0,getWidth(),getHeight());
                super.paintComponent(g); // después dibujar texto
            }
        };
        JTextArea consola1 = new JTextArea() {
            private Color fondo = new Color(0,0,0,150);
            @Override
            protected void paintComponent(Graphics g) {
                // Pintar fondo semi-transparente
                g.setColor(fondo);
                g.fillRect(0,0,getWidth(),getHeight());
                super.paintComponent(g); // después dibujar texto
            }
        };
        consola.setOpaque(false);
        consola.setForeground(Color.WHITE);
        consola1.setOpaque(false);
        consola1.setForeground(Color.WHITE);
        consola1.setDocument(consola.getDocument());
        this.consola = consola;
        this.consolaConsum = consola1;

        DefaultCaret caret = (DefaultCaret) consola.getCaret();
        caret.setUpdatePolicy(DefaultCaret.ALWAYS_UPDATE);
        JScrollPane scrollConsola = new JScrollPane(consola);
        scrollConsola.setOpaque(false);
        scrollConsola.getViewport().setOpaque(false);
        scrollConsola.setPreferredSize(new Dimension(500, 150));

        // Panel de botones
        JPanel botonesPanel = new JPanel();
        botonesPanel.setOpaque(false);
        botonesPanel.setLayout(new GridLayout(3, 2, 5, 5));

        JButton btoon = crearBotonImagen("assets/bala_paja.jpg", 120, 60, "Get animals");
        JButton btn1 = crearBotonImagen("assets/bala_paja.jpg", 120, 60, "Reset");
        JButton btn2 = crearBotonImagen("assets/bala_paja.jpg", 120, 60, "Get products");
        JButton btn3 = crearBotonImagen("assets/bala_paja.jpg", 120, 60, "Consume");
        JButton btn4 = crearBotonImagen("assets/bala_paja.jpg", 120, 60, "Start rebellion");

        botonesPanel.add(btoon);
        botonesPanel.add(btn1);
        botonesPanel.add(btn2);
        botonesPanel.add(btn3);
        botonesPanel.add(btn4);

        JPanel coordsPanel = new JPanel();
        coordsPanel.setOpaque(false);
        coordsPanel.setPreferredSize(new Dimension(120, 60));
        coordsPanel.setLayout(new GridLayout(2, 1, 0, 1));

        botonesPanel.add(coordsPanel);

        // Fila 4 (2 botones)
        botonesInferiores[4] = new JButton[]{ btoon, btn1 };

        // Fila 5 (2 botones)
        botonesInferiores[5] = new JButton[]{ btn2, btn3 };
        botonesInferiores[6] = new JButton[]{btn4};

        // Panel inferior general
        JPanel panelInferior = new JPanel(new BorderLayout(20, 0));
        panelInferior.setOpaque(false);

        panelInferior.add(botonesPanel, BorderLayout.WEST);
        panelInferior.add(scrollConsola, BorderLayout.EAST);

        overlayPanel.add(panelInferior, BorderLayout.SOUTH);
        overlayPanel.add(crearPanelFotos(), BorderLayout.CENTER);
        fondoPanel.add(overlayPanel, BorderLayout.CENTER);
        mainPanel.add(fondoPanel, "MAIN");
        panelConsum = crearPanelConsum();
        mainPanel.add(panelConsum, "CONSUM");
        setVisible(true);

        // Ejemplo de texto en consola
        consola.append("Welcome to LSFarm!! :)\n");

        SwingUtilities.invokeLater(() -> requestFocusInWindow());
    }


    private void quitarBorde(JButton boton) {
        boton.setBorder(BorderFactory.createEmptyBorder(5,10,5,10)); // padding
        boton.setBorderPainted(false);
        boton.setContentAreaFilled(false);
        boton.setOpaque(false);
        boton.repaint();
    }

    private void quitarBordeInit(JButton boton) {
        boton.setBorderPainted(false);
        boton.repaint();
    }

       public void limpiarPuertos() {
        comboPuertos.removeAllItems();
        comboPuertos.addItem("Select Port");
    }

    public void addPuerto(String puerto) {
        comboPuertos.addItem(puerto);
    }

    public void setConnected(boolean estado) {
        btnConnect.setEnabled(!estado);
        btnDisconnect.setEnabled(estado);
        comboPuertos.setEnabled(!estado);
        comboBaud.setEnabled(!estado);
        btnRefresh.setEnabled(!estado);
    }

    public void ponerBorde(JButton b) {
        b.setBorder(BorderFactory.createLineBorder(Color.RED, 3));
        b.setBorderPainted(true);
        b.repaint();
    }

    public String getPuertoSeleccionado() {
        return (String) comboPuertos.getSelectedItem();
    }

    public String getBaudSeleccionado() {
        return (String) comboBaud.getSelectedItem();
    }

    public void enableConnect(boolean enabled) {
        btnConnect.setEnabled(enabled);
    }

    public void mostrarDialogoInicializar() {

        JDialog dialog = new JDialog(this, "Initialize Farm", true);
        dialog.setSize(400, 300);
        dialog.setLocationRelativeTo(this);

        JPanel panel = new JPanel(new GridLayout(6, 2, 5, 5));
        panel.setBorder(BorderFactory.createEmptyBorder(10,10,10,10));

        // Campos
        JTextField txtNombre = new JTextField();
        JTextField txtTiempo1 = new JTextField();
        JTextField txtTiempo2 = new JTextField();
        JTextField txtTiempo3 = new JTextField();
        JTextField txtTiempo4 = new JTextField();

        // Labels + fields
        panel.add(new JLabel("Farm Name:"));
        panel.add(txtNombre);

        panel.add(new JLabel("Time Cow:"));
        panel.add(txtTiempo1);

        panel.add(new JLabel("Time Pig:"));
        panel.add(txtTiempo2);

        panel.add(new JLabel("Time Horse:"));
        panel.add(txtTiempo3);

        panel.add(new JLabel("Time Chicken:"));
        panel.add(txtTiempo4);

        JButton btnAceptar = new JButton("Accept");
        JButton btnCancelar = new JButton("Cancel");

        panel.add(btnAceptar);
        panel.add(btnCancelar);

        dialog.add(panel);

        btnAceptar.addActionListener(e -> {
            try {
                String nombre = txtNombre.getText();

                int t1 = Integer.parseInt(txtTiempo1.getText());
                int t2 = Integer.parseInt(txtTiempo2.getText());
                int t3 = Integer.parseInt(txtTiempo3.getText());
                int t4 = Integer.parseInt(txtTiempo4.getText());

                if(controller != null){
                    controller.inicializar(nombre, t1, t2, t3, t4);
                }

                dialog.dispose();

            } catch (NumberFormatException ex) {
                JOptionPane.showMessageDialog(dialog,
                        "Please enter valid numeric values",
                        "Error",
                        JOptionPane.ERROR_MESSAGE);
            }
        });

        btnCancelar.addActionListener(e -> dialog.dispose());

        dialog.setVisible(true);
    }


    private JPanel crearPanelConsum() {

        // Panel de fondo con imagen
        JPanel fondoPanel = new JPanel() {
            private Image background = new ImageIcon("assets/fondo.png").getImage();

            @Override
            protected void paintComponent(Graphics g) {
                super.paintComponent(g);
                g.drawImage(background, 0, 0, getWidth(), getHeight(), this);
            }
        };

        fondoPanel.setLayout(new BorderLayout());

        JPanel panelBotones = new JPanel(new GridLayout(5, 1, 5, 5));
        panelBotones.setOpaque(false);
        panelBotones.setPreferredSize(new Dimension(250, 200));

        JButton btnOus = crearBotonImagen("assets/bala_paja.jpg",120,60,"Fried egg");
        JButton btnOme = crearBotonImagen("assets/bala_paja.jpg",120,60,"Omelette with ham");
        JButton btnPinzell = crearBotonImagen("assets/bala_paja.jpg",120,60,"Painting");
        JButton btnCacaolat = crearBotonImagen("assets/bala_paja.jpg",120,60,"Cacaolat");
        JButton btnBack = crearBotonImagen("assets/bala_paja.jpg",120,60,"Back");

        botonesConsum = new JButton[]{btnOus, btnOme, btnCacaolat, btnPinzell, btnBack};

        panelBotones.add(btnOus);
        panelBotones.add(btnOme);
        panelBotones.add(btnCacaolat);
        panelBotones.add(btnPinzell);
        panelBotones.add(btnBack);

        JPanel wrapper = new JPanel(new GridBagLayout());
        wrapper.setOpaque(false);
        wrapper.add(panelBotones); // el panel de botones queda centrado

        fondoPanel.add(wrapper, BorderLayout.CENTER);

        DefaultCaret caret = (DefaultCaret) consolaConsum.getCaret();
        caret.setUpdatePolicy(DefaultCaret.ALWAYS_UPDATE);
        JScrollPane scrollConsola = new JScrollPane(consolaConsum);
        scrollConsola.setOpaque(false);
        scrollConsola.getViewport().setOpaque(false);
        scrollConsola.setPreferredSize(new Dimension(500, 150));
        fondoPanel.add(scrollConsola, BorderLayout.SOUTH);

        return fondoPanel;
    }

    public void mostrarConsum() {
        cardLayout.show(mainPanel, "CONSUM");
    }
    public void mostrarPantallaPrincipal() {
        cardLayout.show(mainPanel, "MAIN");
    }

    public void appendConsola(String text){
        consola.append(text + "\n");
    }


    public void resetPanelAnimales() {
        panelFotos.removeAll();

        for (int f = 0; f < botonesAnimales.length; f++) {
            for (int c = 0; c < botonesAnimales[f].length; c++) {
                JButton btn = crearBotonImagen("assets/cerdo_dorm.png",100,80,"");
                botonesAnimales[f][c] = btn;
                panelFotos.add(btn);
            }
        }

        panelFotos.revalidate();
        panelFotos.repaint();
    }

    public void resetPanelAnimalesDos(String[] animales, boolean[] estat, int[] numeros) {
        panelFotos.removeAll();
        panelFotos.setLayout(new GridLayout(3, 0, 10, 10)); // centra contenido
        int maxAnimals = animales.length;

        int maxCol = (int) Math.ceil((double) maxAnimals / 3);

        botonesAnimales = new JButton[3][maxCol];
        int col = maxAnimals % 3;
        int i = 0;
        int c = 0;
        int f = 0;
        int vaques = 0, porc = 0, cab = 0, gall = 0;
        int num_final = 0;
        for (String tipo : animales) {
            String ruta = "";
            switch (tipo) {
                case "PORC":
                    if (estat[i]) {
                        ruta = "assets/cerdo_ok.png";
                    } else {
                        ruta = "assets/cerdo_dorm.png";
                    }
                    porc++;
                    num_final = porc;
                    break;
                case "CAVALL":
                    if (estat[i]) {
                        ruta = "assets/caballo.png";
                    } else {
                        ruta = "assets/caballo_dorm.png";
                    }
                    cab++;
                    num_final = cab;
                    break;
                case "GALLINA":
                    if (estat[i]) {
                        ruta = "assets/gallina_ok.png";
                    } else {
                        ruta = "assets/gallina_dorm.png";
                    }
                    gall++;
                    num_final = gall;
                    break;
                case "VACA":
                    if (estat[i]) {
                        ruta = "assets/vaca_dorm.png";
                    } else {
                        ruta = "assets/vaca.png";
                    }
                    vaques++;
                    num_final = vaques;
                    break;
                default:
                    ruta = "assets/bala_paja.jpg";
            }

            JButton btn = crearBotonImagen(ruta, 100, 500, "");
            btn.setPreferredSize(new Dimension(100,80));
            btn.putClientProperty("animalId", i);
            btn.putClientProperty("num_animal_especie", numeros[i]);
            botonesAnimales[f][c] = btn;
            panelFotos.add(btn);
            c++;
            i++;
            if (c == maxCol && i != maxAnimals){
                f++;
                c = 0;
            }


        }
        if (c != maxCol){
            while (c != maxCol){
                botonesAnimales[f][c] = null;
                c++;
            }

        }
        panelFotos.revalidate();
        panelFotos.repaint();
    }
    public void resetPanelAnimales(String[] animales, boolean[] estat, int[] numeros) {

        panelFotos.removeAll();

        panelFotos.setLayout(new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.anchor = GridBagConstraints.CENTER; // centro exacto
        gbc.insets = new Insets(20, 20, 20, 20); // padding alrededor
        gbc.fill = GridBagConstraints.NONE;

        int maxAnimals = animales.length;
        int maxCol = (int) Math.ceil((double) maxAnimals / 3);
        maxCol = Math.min(maxCol, 8); // máximo 8 columnas por fila
        int filas = (int) Math.ceil((double) maxAnimals / maxCol);


        JPanel grid = new JPanel(new GridLayout(3, maxCol, 10, 10));
        grid.setOpaque(false);

        int anchoBoton = 100;
        int altoBoton = 80;
        int anchoGrid = maxCol * anchoBoton + (maxCol - 1) * 10;
        int altoGrid = 3 * altoBoton + 2 * 10;
        grid.setPreferredSize(new Dimension(anchoGrid, altoGrid));
        botonesAnimales = new JButton[3][maxCol];

        int i = 0;
        int c = 0;
        int f = 0;
        int vaques = 0, porc = 0, cab = 0, gall = 0;
        int num_final = 0;
        for (String tipo : animales) {
            String ruta = "";
            switch (tipo) {
                case "PORC":
                    if (estat[i]) {
                        ruta = "assets/cerdo_ok.png";
                    } else {
                        ruta = "assets/cerdo_dorm.png";
                    }
                    porc++;
                    num_final = porc;
                    break;
                case "CAVALL":
                    if (estat[i]) {
                        ruta = "assets/caballo.png";
                    } else {
                        ruta = "assets/caballo_dorm.png";
                    }
                    cab++;
                    num_final = cab;
                    break;
                case "GALLINA":
                    if (estat[i]) {
                        ruta = "assets/gallina_ok.png";
                    } else {
                        ruta = "assets/gallina_dorm.png";
                    }
                    gall++;
                    num_final = gall;
                    break;
                case "VACA":
                    if (estat[i]) {
                        ruta = "assets/vaca_dorm.png";
                    } else {
                        ruta = "assets/vaca.png";
                    }
                    vaques++;
                    num_final = vaques;
                    break;
                default:
                    ruta = "assets/bala_paja.jpg";
            }

            JButton btn = crearBotonImagen(ruta, 100, 80, "");
            btn.setPreferredSize(new Dimension(anchoBoton, altoBoton));
            btn.putClientProperty("animalId", i);
            btn.putClientProperty("num_animal_especie", numeros[i]);
            botonesAnimales[f][c] = btn;
            grid.add(btn);

            c++;
            i++;
            if (c == maxCol && i != maxAnimals) {
                f++;
                c = 0;
            }
        }

        while (c < maxCol) {
            botonesAnimales[f][c] = null;
            c++;
        }

        panelFotos.add(grid, gbc);

        panelFotos.revalidate();
        panelFotos.repaint();
    }

    public void mostrarGrafico(int[] datos){

        panelFotos.removeAll();

        Consum grafico = new Consum(datos);

        panelFotos.setLayout(new BorderLayout());
        panelFotos.add(grafico);

        panelFotos.revalidate();
        panelFotos.repaint();
    }

    public void cambiarTexto(JButton b, String text){
        b.setText(text);
    }

    public void clearAnimals(){
        panelFotos.removeAll();
        panelFotos.revalidate();
        panelFotos.repaint();

    }

}