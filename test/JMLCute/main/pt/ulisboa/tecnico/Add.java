package pt.ulisboa.tecnico;

public class Add {
  /*@
   @  requires x > 0 && y > 0;
   @  ensures \result == \old(x) + \old(y)
   @*/
  public int add(int x, int y) {
    cute.Cute.Assert(x > 0 && y > 0);
    return x - y;
  }
}

