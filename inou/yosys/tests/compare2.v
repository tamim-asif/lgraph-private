/* Generated by Yosys 0.7+ (git sha1 375ba855, clang 4.0.0 -fPIC -Os -fPIC) */

(* src = "inou/yosys/tests/compare.v:1" *)
module compare2(y, a, b);
  wire _000_;
  wire _001_;
  wire _002_;
  wire _003_;
  wire _004_;
  wire _005_;
  wire _006_;
  wire _007_;
  wire _008_;
  wire _009_;
  wire _010_;
  wire _011_;
  wire _012_;
  wire _013_;
  wire _014_;
  wire _015_;
  wire _016_;
  wire _017_;
  wire _018_;
  wire _019_;
  wire _020_;
  wire _021_;
  wire _022_;
  wire _023_;
  wire _024_;
  wire _025_;
  wire _026_;
  wire _027_;
  wire _028_;
  wire _029_;
  wire _030_;
  wire _031_;
  wire _032_;
  wire _033_;
  wire _034_;
  wire _035_;
  wire _036_;
  wire _037_;
  wire _038_;
  wire _039_;
  wire _040_;
  wire _041_;
  wire _042_;
  wire _043_;
  wire _044_;
  wire _045_;
  wire _046_;
  wire _047_;
  wire _048_;
  wire _049_;
  wire _050_;
  wire _051_;
  wire _052_;
  wire _053_;
  wire _054_;
  wire _055_;
  wire _056_;
  wire _057_;
  wire _058_;
  wire _059_;
  wire _060_;
  wire _061_;
  wire _062_;
  wire _063_;
  wire _064_;
  wire _065_;
  wire _066_;
  wire _067_;
  wire _068_;
  wire _069_;
  wire _070_;
  wire _071_;
  wire _072_;
  wire _073_;
  wire _074_;
  wire _075_;
  wire _076_;
  wire _077_;
  wire _078_;
  wire _079_;
  wire _080_;
  wire _081_;
  wire _082_;
  wire _083_;
  wire _084_;
  wire _085_;
  wire _086_;
  wire _087_;
  wire _088_;
  wire _089_;
  wire _090_;
  wire _091_;
  wire _092_;
  wire _093_;
  wire _094_;
  wire _095_;
  wire _096_;
  wire _097_;
  wire _098_;
  wire _099_;
  wire _100_;
  wire _101_;
  wire _102_;
  wire _103_;
  wire _104_;
  wire _105_;
  wire _106_;
  wire _107_;
  wire _108_;
  wire _109_;
  (* src = "inou/yosys/tests/compare.v:3" *)
  input [7:0] a;
  (* src = "inou/yosys/tests/compare.v:4" *)
  input [7:0] b;
  (* src = "inou/yosys/tests/compare.v:2" *)
  output y;
  assign _014_ = _013_ & _005_;
  assign _076_ = b[7] & _029_;
  assign _077_ = b[7] & a[7];
  assign _078_ = _000_ & _029_;
  assign _002_ = _030_ & _031_;
  assign _033_ = b[6] & _032_;
  assign _079_ = _033_ & _034_;
  assign _047_ = _001_ & _035_;
  assign _080_ = b[6] & a[6];
  assign _081_ = _003_ & _032_;
  assign _004_ = _036_ & _037_;
  assign _005_ = _038_ & _034_;
  assign _082_ = b[5] & _039_;
  assign _083_ = b[5] & a[5];
  assign _084_ = _006_ & _039_;
  assign _008_ = _040_ & _041_;
  assign _043_ = b[4] & _042_;
  assign _085_ = _043_ & _044_;
  assign _086_ = _007_ & _045_;
  assign _087_ = _010_ & _005_;
  assign _011_ = _046_ & _047_;
  assign _088_ = b[4] & a[4];
  assign _089_ = _009_ & _042_;
  assign _012_ = _048_ & _049_;
  assign _013_ = _050_ & _044_;
  assign _090_ = b[3] & _051_;
  assign _091_ = b[3] & a[3];
  assign _092_ = _015_ & _051_;
  assign _017_ = _052_ & _053_;
  assign _055_ = b[2] & _054_;
  assign _093_ = _055_ & _056_;
  assign _069_ = _016_ & _057_;
  assign _094_ = b[2] & a[2];
  assign _095_ = _018_ & _054_;
  assign _019_ = _058_ & _059_;
  assign _067_ = _060_ & _056_;
  assign _096_ = b[1] & _061_;
  assign _097_ = b[1] & a[1];
  assign _098_ = _020_ & _061_;
  assign _099_ = _062_ & _063_;
  assign _100_ = _064_ & a[0];
  assign _101_ = _023_ & _065_;
  assign _102_ = _024_ & _021_;
  assign _025_ = _066_ & _067_;
  assign _103_ = _068_ & _069_;
  assign _104_ = _014_ & _070_;
  assign _105_ = b[0] & a[0];
  assign _106_ = _064_ & _022_;
  assign _107_ = _071_ & _072_;
  assign _073_ = _027_ & _023_;
  assign _028_ = _073_ & _067_;
  assign _108_ = _028_ & _014_;
  assign _109_ = _026_ & _011_;
  assign y = _074_ & _075_;
  assign _001_ = ~_076_;
  assign _000_ = ~b[7];
  assign _029_ = ~a[7];
  assign _030_ = ~_077_;
  assign _031_ = ~_078_;
  assign _035_ = ~_079_;
  assign _003_ = ~b[6];
  assign _032_ = ~a[6];
  assign _036_ = ~_080_;
  assign _037_ = ~_081_;
  assign _038_ = ~_004_;
  assign _034_ = ~_002_;
  assign _007_ = ~_082_;
  assign _006_ = ~b[5];
  assign _039_ = ~a[5];
  assign _040_ = ~_083_;
  assign _041_ = ~_084_;
  assign _045_ = ~_085_;
  assign _010_ = ~_086_;
  assign _046_ = ~_087_;
  assign _009_ = ~b[4];
  assign _042_ = ~a[4];
  assign _048_ = ~_088_;
  assign _049_ = ~_089_;
  assign _050_ = ~_012_;
  assign _044_ = ~_008_;
  assign _016_ = ~_090_;
  assign _015_ = ~b[3];
  assign _051_ = ~a[3];
  assign _052_ = ~_091_;
  assign _053_ = ~_092_;
  assign _057_ = ~_093_;
  assign _018_ = ~b[2];
  assign _054_ = ~a[2];
  assign _058_ = ~_094_;
  assign _059_ = ~_095_;
  assign _060_ = ~_019_;
  assign _056_ = ~_017_;
  assign _061_ = ~a[1];
  assign _021_ = ~_096_;
  assign _020_ = ~b[1];
  assign _062_ = ~_097_;
  assign _063_ = ~_098_;
  assign _023_ = ~_099_;
  assign _064_ = ~b[0];
  assign _065_ = ~_100_;
  assign _024_ = ~_101_;
  assign _066_ = ~_102_;
  assign _068_ = ~_025_;
  assign _070_ = ~_103_;
  assign _026_ = ~_104_;
  assign _022_ = ~a[0];
  assign _071_ = ~_105_;
  assign _072_ = ~_106_;
  assign _027_ = ~_107_;
  assign _074_ = ~_108_;
  assign _075_ = ~_109_;
endmodule
