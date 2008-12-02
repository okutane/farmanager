#include <all_far.h>
#pragma hdrstop

#include "fstdlib.h"

int DECLSPEC FP_Message( unsigned int Flags,CONSTSTR HelpTopic,
                         CONSTSTR *Items,int ItemsNumber,
                         int ButtonsNumber,
                         LPBOOL Delayed /*NULL*/ )
  {  CONSTSTR litems[100];
     int    rc;

     static int CMsgWidth  = 0;
     static int CMsgHeight = 0;

   //Check width and set repaint flag
     int  width = 0;

   //If all lines in one (text)
     if ( IS_FLAG(Flags,FMSG_ALLINONE) ) {
       char *b = (char*)Items,
            *e;
       for( ItemsNumber = 0; (e=strchr(b,'\n')) != NULL; ItemsNumber++, b = e+1 )
         width = Max( width,(int)(e-b) );
     } else
   //Array of lines - check if lines are message id
       for ( rc = 0; rc < (int)ARRAY_SIZE(litems) && rc < ItemsNumber; rc++ ) {
         if ( !FISMSG( Items[rc] ) )
           litems[rc] = FP_GetMsg( FGETID(Items[rc]) );
          else
           litems[rc] = Items[rc];
         width = Max( width,strLen(litems[rc]) );
       }

    //Calc if message need to be redrawn with smaller dimentions
     if ( !CMsgWidth        ||
          width < CMsgWidth ||
          !CMsgHeight       ||
          ItemsNumber < CMsgHeight )
      // Need restore bk
       if ( !ButtonsNumber &&           // No buttons
            (Flags&FMSG_MB_MASK) == 0 ) // No FAR buttons
         FP_Screen::RestoreWithoutNotes();

     rc = FP_Info->Message( FP_Info->ModuleNumber, Flags, HelpTopic,
                            IS_FLAG(Flags,FMSG_ALLINONE) ? Items : litems,
                            ItemsNumber, ButtonsNumber );

     if ( Delayed )
       *Delayed = ButtonsNumber || (Flags&FMSG_MB_MASK) != 0;

     CMsgWidth  = width;
     CMsgHeight = ItemsNumber;

  return rc;
}
