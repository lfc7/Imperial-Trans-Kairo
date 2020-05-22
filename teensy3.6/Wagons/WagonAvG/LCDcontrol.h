/* Custom char ***************************************************************/
byte customChar1[8] =
{
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b01110
};
byte customChar2[8] =
{
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b01110,
  0b01110
};
byte customChar3[8] =
{
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b01110,
  0b01110,
  0b01110
};
byte customChar4[8] =
{
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b01110,
  0b01110,
  0b01110,
  0b01110
};
byte customChar5[8] =
{
  0b00000,
  0b00000,
  0b00000,
  0b01110,
  0b01110,
  0b01110,
  0b01110,
  0b01110
};
byte customChar6[8] =
{
  0b00000,
  0b00000,
  0b01110,
  0b01110,
  0b01110,
  0b01110,
  0b01110,
  0b01110
};
byte customChar7[8] =
{
  0b00000,
  0b01110,
  0b01110,
  0b01110,
  0b01110,
  0b01110,
  0b01110,
  0b01110
};
byte customChar8[8] =
{
  0b01110,
  0b01110,
  0b01110,
  0b01110,
  0b01110,
  0b01110,
  0b01110,
  0b01110
};

byte* customChars[8]=
{
  customChar1,
  customChar2,
  customChar3,
  customChar4,
  customChar5,
  customChar6,
  customChar7,
  customChar8
};

/* ROUTINES ********************************************************************/
void displayInit()
{
  /*
  //change baudRate
  Serial1.write(0xFE);
  Serial1.write(0x61);
  Serial1.write(0x08);
  */
  //delay(30);
/*
  Serial1.begin(19200);
  delay(30);
  Serial1.write(0xFE);
  delay(1);
  Serial1.write(0x61);
  delay(1);
  Serial1.write(0x06);
  Serial1.flush();
  delay(1);
  delay(40);
  */
  
  
  //delay(1);
  
  
  //display on
  Serial1.write(0xFE);
  Serial1.write(0x41);
  Serial1.flush();
  delay(1);

  //load custom char
  for (byte i = 0; i < 8; i++)
  {
    Serial1.write(0xFE);
    Serial1.write(0x54);
    Serial1.write(i); //send char add
    Serial1.write(customChars[i], 8); //send char bytes
    Serial1.flush();
    delay(10); //need 200us
  }

  //clear display
  Serial1.write(0xFE);
  Serial1.write(0x51);
  Serial1.flush();

}

void displayClear()
{
  //clear display
  Serial1.write(0xFE);
  Serial1.write(0x51);
  Serial1.flush();
  delay(2); //need 1.5ms
}

void displayHome()
{
  //home display
  Serial1.write(0xFE);
  Serial1.write(0x46);
  Serial1.flush();
  delay(2); //need 1.5ms
}

void displaySetCursor(byte line , byte pos)
{
  byte realPos = 0;
  const byte lineStart[4] = {0x00, 0x40, 0x14, 0x54};
  realPos = constrain(pos, 0x00, 0x13) + lineStart[line];
  //set cursor
  Serial1.write(0xFE);
  Serial1.write(0x45);
  Serial1.write(realPos);
  Serial1.flush();
  delay(2);
}

void displaySetLine(byte line)
{
  displaySetCursor(line , 0);
}

void displayClearLine(byte line)
{
  displaySetLine(line);

  for (byte i = 0; i < 20; i++)
  {
    Serial1.write(' ');
  }
  Serial1.flush();
  
  //Serial1.print(F("                    "));
  displaySetLine(line);

}

void displaySetContrast(byte val)
{
  //contrast
  Serial1.write(0xFE);
  Serial1.write(0x52);
  Serial1.write(constrain(val, 1, 50));
  Serial1.flush();

}

void displaySetBackLight(byte val)
{
  //contrast
  Serial1.write(0xFE);
  Serial1.write(0x53);
  Serial1.write(byte(constrain(val, 1, 8)));
  Serial1.flush();

}

