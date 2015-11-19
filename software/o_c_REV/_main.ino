/*
* 
* main loop
*
*/

void _loop()
{
   UI();
  
   if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); }  
 
   if (_ENC && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) encoders();
  
   if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); } 
    
   if (_ADC) CV();
  
   if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); } 
  
   buttons(BUTTON_TOP);
  
   if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); } 
  
   buttons(BUTTON_BOTTOM);
  
   if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); } 
  
   buttons(BUTTON_LEFT);
  
   if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); } 
  
   buttons(BUTTON_RIGHT);
  
   if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); }  
   
   if (UI_MODE) timeout(); 
 
   if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); } 
}
