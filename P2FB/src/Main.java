import javax.swing.*;

public class Main {

    public static void main(String[] args) {

        SwingUtilities.invokeLater(() -> {
            UI ui = new UI();
            Controller controller = new Controller(ui);

            ui.setController(controller);
        });
    }
}