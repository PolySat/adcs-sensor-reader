#include <polysat/cmd.h>
#include "polypkt/adcs-sensor-reader-pkt.h"

int main(int argc, char **argv)
{
   enum XDR_PRINT_STYLE style;

   IPC_ADCS_forcelink();
   return CMD_send_command_line_command_blocking(argc, argv, NULL,
      CMD_print_response, &style, 3000, "adcs", &style);
}
