import javax.swing.*;
import java.awt.*;
import java.awt.geom.AffineTransform;

public class Consum extends JPanel {

    private int[] datos;
    private String[] nombres;

    public Consum(int[] datos) {

        this.datos = datos;

        nombres = new String[]{
                "MILK",
                "HAM",
                "EGGS",
                "BRUSH"
        };

        setLayout(new BorderLayout());
        add(new HistogramaPanel(), BorderLayout.CENTER);
    }

    class HistogramaPanel extends JPanel {

        @Override
        protected void paintComponent(Graphics g) {
            super.paintComponent(g);

            Color[] colors = {
                    Color.RED,
                    Color.PINK,
                    Color.BLUE,
                    Color.YELLOW
            };

            int ancho = getWidth();
            int alto = getHeight();
            int margen = 40;

            int numBarras = datos.length;
            int anchoBarra = (ancho - 2 * margen) / numBarras;

            int maxAltura = 0;
            for (int valor : datos)
                if (valor > maxAltura) maxAltura = valor;

            for (int i = 0; i < numBarras; i++) {

                int alturaBarra = (int)((double)datos[i]/maxAltura*(alto-2*margen));
                int x = margen + i * anchoBarra;
                int y = alto - margen - alturaBarra;

                g.setColor(colors[i]);
                g.fillRect(x, y, anchoBarra-8, alturaBarra);

                g.setColor(Color.BLACK);
                g.drawString(String.valueOf(datos[i]), x+10, y-5);

                g.drawString(nombres[i], x+10, alto-margen+15);
            }

            Graphics2D g2 = (Graphics2D) g;
            AffineTransform original = g2.getTransform();
            g2.rotate(-Math.PI/2);
            g2.drawString("N Products", -getHeight()/2, margen-50);
            g2.setTransform(original);
        }
    }
}