package pt.ulisboa.tecnico;

public class Main {
    /*@
     @ requires x > 0 && y > 0;
     @ ensures \result == x + y;
     @*/
    private int add(int x, int y){
        return x + y;
    }
    
    public static void main(String[] args) {
	new Main().add(1,0);
    }
}
