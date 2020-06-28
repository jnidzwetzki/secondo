import unittest
import secondodb.api.secondoapi as api
from secondodb.api.algebras import secondospatialalgebra as spatial


class TestSpatialAlgebra(unittest.TestCase):

    def setUp(self):
        HOST = '127.0.0.1'
        PORT = '1234'

        self.connection = api.connect(HOST, PORT, database='BERLINTEST')
        self.cursor = self.connection.cursor()

    def test_parse_point(self):
        response = self.cursor.execute_simple_query('alexanderplatz')
        self.assertIsInstance(response, object)
        self.assertIsInstance(response.x, float)
        self.assertIsInstance(response.y, float)

    def test_convert_point_to_list_exp_str(self):

        comp_string = '(11068.0 12895.0)'

        resp_object = self.cursor.execute_simple_query('alexanderplatz')
        result = spatial.convert_point_to_list_exp_str(resp_object)
        self.assertIsInstance(result, str)
        self.assertEqual(result, comp_string)

    def test_parse_points(self):
        response = self.cursor.execute_simple_query('Sehenswuerdaspoints')
        self.assertIsInstance(response, list)
        self.assertIsInstance(response[0].x, float)
        self.assertIsInstance(response[0].y, float)

    def test_convert_points_to_list_exp_str(self):
        comp_string = '((-15951.0 -1010.0) (-15928.0 -1543.0) (-15738.0 -989.0) (-15547.0 -284.0) (-15397.0 -412.0) ' \
                      '(-14863.0 -560.0) (-14822.0 -70.0) (-14715.0 -176.0) (-14479.0 -666.0) (-14286.0 -794.0) ' \
                      '(-14203.0 -26.0) (-13560.0 -1091.0) (-13261.0 -813.0) (-12902.0 808.0) (-12899.0 -684.0) ' \
                      '(-12818.0 1064.0) (-12770.0 -982.0) (-12757.90884024587 -739.3294107337315) (-12598.0 -1515.0) ' \
                      '(-12529.0 -3392.0) (-12280.0 -320.0) (-12090.0 362.0) (-12070.0 1065.0) (-10956.0 -701.0) ' \
                      '(-10535.0 1986.0) (-10499.74351206199 -189.8974574043208) (-10364.0 1709.0) ' \
                      '(-10362.38552372964 513.3754428573247) (-8878.0 1726.0) (-8845.953332540466 1628.722308116028) ' \
                      '(-8675.629427008349 2430.892959976968) (-5653.0 2807.0) (-5609.799127430238 2694.620297575085) ' \
                      '(-3613.0 7768.0) (-3445.03723131236 14100.82764869365) (-2637.372259918126 14782.12327082212) ' \
                      '(-1461.587879793187 11963.53735024224) (-1265.0 11792.0) (-752.8206599982475 11870.13391817624) ' \
                      '(-456.1274052003658 11436.08267504601) (450.4353177931618 19902.82907585223) (831.0 6656.0) ' \
                      '(1285.571886853866 10392.16196372013) (1395.458277519748 10309.74717072072) ' \
                      '(1587.759461185042 10831.70752638366) (1648.196976051277 20886.31227231187) ' \
                      '(1769.072005783747 11216.30989371424) (1791.049283916924 12540.44090123812) (1877.0 10888.0) ' \
                      '(2021.810704315276 4496.757104495552) (2021.810704315276 10798.74160918389) ' \
                      '(2604.208574844452 16369.98161594412) (2626.0 7836.0) (2720.0 7066.0) ' \
                      '(2763.543841309981 5024.211779691786) (3010.788220308215 12639.33865283742) ' \
                      '(3131.663250040686 12441.54314963883) (3900.867984701861 11743.76456891048) ' \
                      '(3917.350943301743 12298.69084177318) (4574.0 8247.0) (4768.970470962329 14935.96421775435) ' \
                      '(4833.0 5628.0) (4866.0 5483.0) (4933.800056961152 11529.48610711201) (4970.0 10554.0) ' \
                      '(4983.2489327608 3947.325151166141) (5143.0 6093.0) (5181.044435959388 11716.29297124401) ' \
                      '(5192.033075025975 11452.56563364589) (5301.0 11099.0) (5318.40242429174 11029.50302958225) ' \
                      '(5334.0 6537.0) (5436.0 11107.0) (5518.0 773.0) (5754.0 10934.0) (5877.0 7360.0) ' \
                      '(5950.249170620562 10886.6507217166) (6142.550354285856 10573.47450831883) ' \
                      '(6142.550354285856 11062.46894678201) (6241.44810588515 15128.26540141965) ' \
                      '(6263.425384018326 14996.40173262059) (6378.806094217503 8721.88882559872) ' \
                      '(6505.175443483267 12441.54314963883) (6615.061834149149 6254.939355149666) (6749.0 12015.0) ' \
                      '(6823.845976414325 5694.518762753667) (6933.732367080207 12337.15107850624) ' \
                      '(6955.709645213384 5672.54148462049) (6988.675562413148 10282.27557305425) ' \
                      '(7109.550592145619 11155.87237884801) (7254.0 8407.0) (7274.380178144442 9579.0026727926) ' \
                      '(7443.0 -7573.0) (7505.141598542795 11276.74740858048) (7724.914379874559 12529.45226217154) ' \
                      '(7928.204202606441 11199.82693511436) (7977.653078406088 12688.78752863707) ' \
                      '(8076.550830005382 11540.4747461786) (8158.965623004793 11227.29853278083) ' \
                      '(8224.897457404322 11254.7701304473) (8279.840652737263 11089.94054444848) ' \
                      '(8301.81793087044 11545.96906571189) (8345.772487136792 11249.27581091401) ' \
                      '(8356.76112620338 8029.604564403662) (8384.23272386985 12683.29320910377) ' \
                      '(8488.624795002439 12501.98066450506) (8532.579351268792 13254.70244056636) ' \
                      '(8587.522546601733 10265.79261445436) (8593.0 12243.0) (8647.96006146797 13831.60599156224) ' \
                      '(8697.408937267615 13419.53202656518) (8774.329410733733 13815.12303296236) ' \
                      '(8818.283967000087 11496.52018991224) (8867.732842799733 10661.38362085154) ' \
                      '(8873.227162333027 13155.80468896706) (8878.721481866321 11326.19628438013) ' \
                      '(8906.193079532792 11194.33261558107) (8911.0 9073.0) (8927.0 10842.0) ' \
                      '(9208.380653863967 9787.786815057776) (9230.357931997143 12243.74764644024) ' \
                      '(9241.346571063732 11534.9804266453) (9277.0 10458.0) (9317.0 8702.0) ' \
                      '(9329.255683596437 13117.34445223401) (9455.625032862203 8518.599002866838) (9483.0 9041.0) ' \
                      '(9560.01710399479 11969.03166977554) (9576.500062594672 12067.92942137483) ' \
                      '(9614.960299327731 11875.62823770954) (9631.443257927614 10084.48006985566) ' \
                      '(9642.431896994201 12282.2078831733) (9675.397814193966 12474.50906683859) ' \
                      '(9691.88077279385 8749.36042326519) (9736.0 13221.0) (9746.82396812679 12315.17380037307) ' \
                      '(9763.306926726673 12864.60575370248) (9768.801246259965 12199.79309017389) ' \
                      '(9785.284204859849 9271.32077892813) (9796.272843926437 12463.52042777201) ' \
                      '(9823.744441592908 10743.79841385095) (9834.733080659496 10633.91202318507) ' \
                      '(9862.204678325967 12793.17959976965) (9900.664915059026 12469.0147473053) ' \
                      '(9988.57402759173 12782.19096070306) (10021.5399447915 12617.36137470424) ' \
                      '(10070.98882059114 12089.90669950801) (10160.0 18152.0) (10175.38089172373 12589.88977703777) ' \
                      '(10202.8524893902 12007.4919065086) (10240.0 13151.0) (10257.79568472314 12425.06019103895) ' \
                      '(10336.0 14320.0) (10373.17639492232 12271.21924410671) (10384.16503398891 19205.05049512388) ' \
                      '(10411.63663165538 15452.430253884) (10422.62527072196 12194.29877064059) ' \
                      '(10444.60254885514 15188.70291628588) (10592.94917625408 12754.71936303659) ' \
                      '(10614.92645438726 12353.63403710612) (10686.35260832008 12518.46362310495) ' \
                      '(10735.80148411973 9842.730010390718) (10749.0 12785.0) (10874.0 13439.0) (10957.0 9368.0) ' \
                      '(11120.40385145032 11974.52598930883) (11176.0 15771.0) (11219.30160304961 12776.69664116977) ' \
                      '(11302.0 14228.0) (11307.21071558232 8683.428588865661) (11785.2165149789 10941.59391704954) ' \
                      '(11950.04610097773 15293.09498741847) (11963.0 11100.0) (12290.69391204196 10765.77569198413) ' \
                      '(12504.0 13364.0) (12763.20539190525 13375.57747029883) (12851.11450443796 4623.126453761316) ' \
                      '(12851.11450443796 11067.9632663153) (13224.72823270196 13496.4525000313) ' \
                      '(13493.94988983337 4754.990122560375) (13658.77947583219 13820.61735249565) ' \
                      '(13829.10338136431 10183.37782145495) (14197.22279009502 3606.677340101907) ' \
                      '(14977.41616382278 9068.030956196248) (15362.01853115337 8974.627524130248) ' \
                      '(15427.9503655529 9177.91734686213) (15534.0 9563.0) (15642.22882735137 9150.44574919566) ' \
                      '(18488.28634559771 -1821.71035879267) (18504.76930419759 11139.38942024813) ' \
                      '(20158.55948371912 6463.723497414842) (20356.35498691771 6359.331426282254) ' \
                      '(20850.84374491418 11765.74184704366) (22021.13380550582 4430.825270096023) ' \
                      '(22152.99747430488 4612.137814694728) (22165.0 30737.0) (22699.0 30674.0) (25658.0 1537.0))'
        resp_object = self.cursor.execute_simple_query('Sehenswuerdaspoints')
        result = spatial.convert_points_to_list_exp_str(resp_object)
        self.assertIsInstance(result, str)
        self.assertEqual(result, comp_string)

    def test_parse_line(self):
        response = self.cursor.execute_simple_query('PotsdamLine')
        self.assertIsInstance(response, object)
        self.assertIsInstance(response.segments, list)
        self.assertIsInstance(response.segments[0].x1, float)
        self.assertIsInstance(response.segments[0].x2, float)
        self.assertIsInstance(response.segments[0].y1, float)
        self.assertIsInstance(response.segments[0].y2, float)

    def test_convert_line_to_list_exp_str(self):

        comp_string = '((-21121.0 172.0 -20630.0 88.0) (-21121.0 172.0 -21103.0 1729.0) ' \
                      '(-21103.0 1729.0 -20913.0 2262.0) (-20913.0 2262.0 -20273.0 2520.0) ' \
                      '(-20630.0 88.0 -20290.0 792.0) (-20290.0 792.0 -20185.0 1688.0) ' \
                      '(-20277.0 4119.0 -20273.0 2520.0) (-20277.0 4119.0 -19937.0 4717.0) ' \
                      '(-20185.0 1688.0 -19758.0 1582.0) (-19937.0 4717.0 -19104.0 4676.0) ' \
                      '(-19758.0 1582.0 -19117.0 1520.0) (-19117.0 1520.0 -18947.0 1755.0) ' \
                      '(-19104.0 4676.0 -17780.0 4508.0) (-18947.0 1755.0 -18306.0 1649.0) ' \
                      '(-18761.0 -4215.0 -18035.0 -4512.0) (-18761.0 -4215.0 -18294.0 -3105.0) ' \
                      '(-18511.0 -2018.0 -18489.0 -2253.0) (-18511.0 -2018.0 -18233.0 -1911.0) ' \
                      '(-18489.0 -2253.0 -18232.0 -2444.0) (-18338.0 -2785.0 -18294.0 -3105.0) ' \
                      '(-18338.0 -2785.0 -18232.0 -2444.0) (-18306.0 1649.0 -18284.0 1223.0) ' \
                      '(-18284.0 1223.0 -17578.0 883.0) (-18233.0 -1911.0 -18065.0 -1100.0) ' \
                      '(-18065.0 -1100.0 -18003.0 -162.0) (-18035.0 -4512.0 -17585.0 -4959.0) ' \
                      '(-18003.0 -162.0 -17705.0 414.0) (-17780.0 4508.0 -17074.0 4126.0) ' \
                      '(-17705.0 414.0 -17578.0 883.0) (-17585.0 -4959.0 -17308.0 -4788.0) ' \
                      '(-17308.0 -4788.0 -16473.0 -5682.0) (-17074.0 4126.0 -16070.0 3829.0) ' \
                      '(-16473.0 -5682.0 -15809.0 -6469.0) (-16070.0 3829.0 -15643.0 3788.0) ' \
                      '(-15809.0 -6469.0 -14997.0 -6958.0) (-15643.0 3788.0 -14767.0 3576.0) ' \
                      '(-14997.0 -6958.0 -14591.0 -6915.0) (-14767.0 3576.0 -13121.0 2918.0) ' \
                      '(-14591.0 -6915.0 -13800.0 -7404.0) (-13800.0 -7404.0 -13608.0 -7254.0) ' \
                      '(-13608.0 -7254.0 -13288.0 -7339.0) (-13419.0 -6102.0 -12927.0 -6463.0) ' \
                      '(-13419.0 -6102.0 -13270.0 -5824.0) (-13288.0 -7339.0 -13010.0 -7231.0) ' \
                      '(-13272.0 3302.0 -12908.0 2940.0) (-13272.0 3302.0 -13251.0 3537.0) ' \
                      '(-13270.0 -5824.0 -12608.0 -5887.0) (-13251.0 3537.0 -12569.0 3986.0) ' \
                      '(-13121.0 2918.0 -12908.0 2940.0) (-13010.0 -7231.0 -12927.0 -6463.0) ' \
                      '(-12608.0 -5887.0 -11882.0 -6141.0) (-12569.0 3986.0 -12163.0 3880.0) ' \
                      '(-12163.0 3880.0 -10073.0 4887.0) (-11882.0 -6141.0 -11284.0 -5884.0) ' \
                      '(-11284.0 -5884.0 -10751.0 -5883.0) (-10751.0 -5883.0 -10195.0 -6138.0) ' \
                      '(-10195.0 -6138.0 -9767.0 -6329.0) (-10073.0 4887.0 -9453.0 4611.0) ' \
                      '(-9767.0 -6329.0 -8828.0 -6263.0) (-8828.0 -6263.0 -8209.0 -6176.0) ' \
                      '(-8209.0 -6176.0 -7675.0 -6367.0) (-7675.0 -6367.0 -7356.0 -5940.0) ' \
                      '(-7356.0 -5940.0 -7336.0 -5108.0) (-7336.0 -5108.0 -6633.0 -4467.0) ' \
                      '(-6700.0 -3358.0 -6633.0 -4467.0) (-6700.0 -3358.0 -6510.0 -2611.0) ' \
                      '(-6510.0 -2611.0 -6276.0 -2099.0) (-6276.0 -2099.0 -5829.0 -1736.0))'

        resp_object = self.cursor.execute_simple_query('PotsdamLine')
        result = spatial.convert_line_to_list_exp_str(resp_object)
        self.assertIsInstance(result, str)
        self.assertEqual(result, comp_string)

    def test_parse_region_without_holecycles(self):
        response = self.cursor.execute_simple_query('koepenick')
        self.assertIsInstance(response, object)
        self.assertIsInstance(response.faces, list)
        self.assertIsInstance(response.faces[0].outercycle, list)
        self.assertIsInstance(response.faces[0].holecycles, list)
        self.assertIsInstance(response.faces[0].outercycle[0].x, float)
        self.assertIsInstance(response.faces[0].outercycle[0].y, float)

    def test_convert_region_to_list_exp_str_without_holecycles(self):

        comp_string = '((((22578.0 655.0) (22189.0 1058.0) (22447.0 1239.0) (22573.0 990.0) (22807.0 1057.0) ' \
                      '(22934.0 751.0) (22981.0 508.0) (23315.0 301.0) (23460.0 511.0) (23246.0 726.0) (23270.0 754.0) ' \
                      '(23519.0 604.0) (23537.0 765.0) (23677.0 780.0) (24012.0 691.0) (24217.0 555.0) (24566.0 489.0) ' \
                      '(24590.0 541.0) (24622.0 540.0) (24664.0 521.0) (24702.0 453.0) (24737.0 407.0) (24782.0 383.0) ' \
                      '(24839.0 372.0) (25008.0 373.0) (25167.0 396.0) (25264.0 443.0) (25344.0 482.0) (25445.0 479.0) ' \
                      '(25510.0 498.0) (25528.0 493.0) (25542.0 420.0) (25559.0 390.0) (25611.0 372.0) (25621.0 291.0) ' \
                      '(25604.0 174.0) (25601.0 95.0) (25631.0 -46.0) (25643.0 -61.0) (25687.0 -126.0) (25759.0 -194.0) ' \
                      '(25799.0 -225.0) (25797.0 -250.0) (25771.0 -270.0) (25765.0 -344.0) (25786.0 -393.0) ' \
                      '(25860.0 -512.0) (25987.0 -593.0) (25997.0 -615.0) (25952.0 -695.0) (26012.0 -825.0) ' \
                      '(25951.0 -981.0) (25984.0 -1237.0) (26056.0 -1591.0) (26353.0 -1806.0) (26460.0 -1878.0) ' \
                      '(26773.0 -1928.0) (26793.0 -2055.0) (27012.0 -2310.0) (26876.0 -2488.0) (27073.0 -2723.0) ' \
                      '(26915.0 -2783.0) (26915.0 -2808.0) (26339.0 -2929.0) (25875.0 -2152.0) (24750.0 -2730.0) ' \
                      '(24819.0 -2963.0) (24730.0 -3026.0) (24642.0 -2917.0) (24513.0 -2982.0) (24424.0 -2823.0) ' \
                      '(24488.0 -2750.0) (24487.0 -2706.0) (24375.0 -2501.0) (24280.0 -2412.0) (24247.0 -2295.0) ' \
                      '(24132.0 -2174.0) (24097.0 -2172.0) (23999.0 -2034.0) (23779.0 -1979.0) (23679.0 -1804.0) ' \
                      '(23702.0 -1712.0) (23681.0 -1649.0) (23634.0 -1650.0) (23583.0 -1540.0) (23618.0 -1275.0) ' \
                      '(23522.0 -1139.0) (23512.0 -1036.0) (23556.0 -931.0) (23528.0 -860.0) (23541.0 -778.0) ' \
                      '(23346.0 -728.0) (23220.0 -480.0) (23068.0 -495.0) (22957.0 -338.0) (22953.0 -278.0) ' \
                      '(23139.0 45.0) (22915.0 396.0))) (((23393.0 3759.0) (23214.0 3839.0) (23291.0 3907.0) ' \
                      '(23446.0 3878.0) (23734.0 3870.0) (23744.0 3831.0) (23953.0 3840.0) (23954.0 3872.0) ' \
                      '(24327.0 3885.0) (24323.0 4265.0) (24146.0 4286.0) (24221.0 4638.0) (24068.0 4701.0) ' \
                      '(23912.0 4878.0) (23886.0 5047.0) (23991.0 5045.0) (24096.0 4978.0) (24207.0 4921.0) ' \
                      '(24281.0 4911.0) (24402.0 4866.0) (24440.0 4838.0) (24472.0 4819.0) (24510.0 4780.0) ' \
                      '(24572.0 4757.0) (24719.0 4647.0) (24858.0 4581.0) (24915.0 4557.0) (24972.0 4509.0) ' \
                      '(25053.0 4432.0) (25129.0 4421.0) (25187.0 4437.0) (25260.0 4471.0) (25330.0 4529.0) ' \
                      '(25451.0 4510.0) (25460.0 4535.0) (25577.0 4557.0) (25602.0 4543.0) (25588.0 4493.0) ' \
                      '(25512.0 4376.0) (25427.0 4231.0) (25419.0 4159.0) (25396.0 4102.0) (25405.0 4048.0) ' \
                      '(25375.0 3936.0) (25325.0 3866.0) (25336.0 3824.0) (25313.0 3641.0) (25320.0 3375.0) ' \
                      '(25390.0 3068.0) (25482.0 2915.0) (25542.0 2881.0) (25673.0 2828.0) (25845.0 2708.0) ' \
                      '(25885.0 2660.0) (25957.0 2644.0) (26018.0 2645.0) (26157.0 2680.0) (26309.0 2704.0) ' \
                      '(26357.0 2724.0) (26414.0 2721.0) (26461.0 2695.0) (26577.0 2660.0) (26779.0 2638.0) ' \
                      '(26882.0 2613.0) (26968.0 2620.0) (26995.0 2605.0) (27131.0 2562.0) (27203.0 2538.0) ' \
                      '(27206.0 2479.0) (27276.0 2461.0) (27371.0 2483.0) (27469.0 2572.0) (27560.0 2687.0) ' \
                      '(27609.0 2779.0) (27652.0 2842.0) (27738.0 2918.0) (27816.0 2942.0) (27868.0 3024.0) ' \
                      '(28004.0 3101.0) (28179.0 3174.0) (28374.0 3240.0) (28434.0 3276.0) (28483.0 3277.0) ' \
                      '(28524.0 3313.0) (28600.0 3337.0) (28707.0 3361.0) (28793.0 3363.0) (28894.0 3333.0) ' \
                      '(28860.0 3214.0) (28836.0 3201.0) (28862.0 3121.0) (28819.0 3073.0) (28846.0 2980.0) ' \
                      '(28987.0 2894.0) (29152.0 2873.0) (29308.0 2940.0) (29408.0 2940.0) (29481.0 2979.0) ' \
                      '(29577.0 2944.0) (29547.0 2785.0) (29642.0 2679.0) (30378.0 2360.0) (30591.0 2372.0) ' \
                      '(30627.0 2299.0) (30713.0 2274.0) (30769.0 2304.0) (30887.0 2285.0) (30940.0 2222.0) ' \
                      '(31058.0 2205.0) (31847.0 2378.0) (32148.0 1970.0) (32178.0 1840.0) (32329.0 1750.0) ' \
                      '(32380.0 1672.0) (32445.0 1634.0) (32212.0 1542.0) (31979.0 1416.0) (31709.0 1196.0) ' \
                      '(31516.0 952.0) (30998.0 -75.0) (31114.0 -104.0) (31226.0 -72.0) (31333.0 -131.0) ' \
                      '(31316.0 -174.0) (31244.0 -138.0) (31162.0 -172.0) (31000.0 -477.0) (30788.0 -430.0) ' \
                      '(30640.0 -431.0) (30395.0 -522.0) (30235.0 -704.0) (30179.0 -845.0) (30197.0 -867.0) ' \
                      '(30114.0 -906.0) (30077.0 -897.0) (29999.0 -904.0) (29897.0 -940.0) (29833.0 -949.0) ' \
                      '(29787.0 -973.0) (29718.0 -974.0) (29636.0 -1038.0) (29555.0 -1101.0) (29511.0 -1243.0) ' \
                      '(29490.0 -1275.0) (29439.0 -1298.0) (29330.0 -1353.0) (29223.0 -1515.0) (29247.0 -1683.0) ' \
                      '(29290.0 -1741.0) (29202.0 -1758.0) (29123.0 -1762.0) (28995.0 -1836.0) (28941.0 -1840.0) ' \
                      '(28930.0 -1889.0) (28975.0 -1921.0) (28885.0 -1962.0) (28792.0 -2166.0) (28764.0 -2263.0) ' \
                      '(28635.0 -2286.0) (28560.0 -2334.0) (28466.0 -2317.0) (28312.0 -2209.0) (28263.0 -2114.0) ' \
                      '(28177.0 -2015.0) (28139.0 -2030.0) (28119.0 -2142.0) (28138.0 -2228.0) (28159.0 -2264.0) ' \
                      '(28128.0 -2324.0) (27980.0 -2387.0) (27938.0 -2464.0) (27912.0 -2428.0) (27887.0 -2394.0) ' \
                      '(27751.0 -2338.0) (27735.0 -2293.0) (27720.0 -2173.0) (27731.0 -2015.0) (27771.0 -1718.0) ' \
                      '(27773.0 -1506.0) (27783.0 -1399.0) (27764.0 -1328.0) (27809.0 -1258.0) (27795.0 -1155.0) ' \
                      '(27736.0 -1055.0) (27758.0 -1035.0) (27805.0 -945.0) (27798.0 -874.0) (27823.0 -688.0) ' \
                      '(27865.0 -672.0) (27947.0 -508.0) (28002.0 -477.0) (28085.0 -352.0) (28192.0 -293.0) ' \
                      '(28227.0 -220.0) (28225.0 -16.0) (28249.0 4.0) (28270.0 246.0) (28337.0 326.0) (28480.0 403.0) ' \
                      '(28485.0 497.0) (28513.0 569.0) (28661.0 526.0) (28811.0 514.0) (29164.0 976.0) (29010.0 1148.0) ' \
                      '(29625.0 1946.0) (29300.0 2213.0) (29452.0 2411.0) (29210.0 2603.0) (29079.0 2521.0) ' \
                      '(28769.0 2788.0) (28682.0 2631.0) (28922.0 2431.0) (28721.0 2197.0) (28449.0 2413.0) ' \
                      '(28344.0 2300.0) (28387.0 2242.0) (28260.0 2083.0) (28189.0 1769.0) (27945.0 1714.0) ' \
                      '(27666.0 1481.0) (27537.0 1416.0) (27189.0 1406.0) (27031.0 805.0) (27178.0 621.0) ' \
                      '(27588.0 253.0) (27699.0 354.0) (27911.0 184.0) (27858.0 57.0) (28056.0 -125.0) ' \
                      '(28071.0 -160.0) (28018.0 -181.0) (27965.0 -226.0) (27887.0 -262.0) (27863.0 -295.0) ' \
                      '(27855.0 -354.0) (27824.0 -387.0) (27751.0 -399.0) (27688.0 -435.0) (27652.0 -485.0) ' \
                      '(27629.0 -554.0) (27625.0 -609.0) (27624.0 -670.0) (27603.0 -710.0) (27607.0 -779.0) ' \
                      '(27611.0 -848.0) (27634.0 -885.0) (27613.0 -937.0) (27586.0 -1014.0) (27587.0 -1071.0) ' \
                      '(27607.0 -1102.0) (27587.0 -1187.0) (27577.0 -1268.0) (27570.0 -1308.0) (27572.0 -1370.0) ' \
                      '(27594.0 -1394.0) (27595.0 -1433.0) (27586.0 -1483.0) (27600.0 -1532.0) (27617.0 -1534.0) ' \
                      '(27600.0 -1559.0) (27522.0 -1578.0) (27508.0 -1571.0) (27438.0 -1535.0) (27385.0 -1566.0) ' \
                      '(27371.0 -1606.0) (27367.0 -1660.0) (27365.0 -1685.0) (27369.0 -1737.0) (27308.0 -1728.0) ' \
                      '(27320.0 -1642.0) (27336.0 -1575.0) (27357.0 -1513.0) (27364.0 -1428.0) (27358.0 -1364.0) ' \
                      '(27325.0 -1212.0) (27298.0 -1191.0) (27233.0 -1133.0) (27216.0 -1054.0) (27212.0 -1000.0) ' \
                      '(27166.0 -885.0) (27149.0 -814.0) (27112.0 -780.0) (27040.0 -752.0) (26961.0 -742.0) ' \
                      '(26938.0 -683.0) (26911.0 -600.0) (26846.0 -547.0) (26784.0 -494.0) (26749.0 -468.0) ' \
                      '(26712.0 -370.0) (26695.0 -348.0) (26668.0 -354.0) (26678.0 -383.0) (26633.0 -438.0) ' \
                      '(26630.0 -500.0) (26596.0 -525.0) (26594.0 -456.0) (26595.0 -387.0) (26586.0 -314.0) ' \
                      '(26451.0 -193.0) (26402.0 -214.0) (26340.0 -169.0) (26277.0 -81.0) (26178.0 49.0) ' \
                      '(26143.0 73.0) (26116.0 92.0) (26080.0 138.0) (26032.0 208.0) (26041.0 236.0) (26031.0 265.0) ' \
                      '(26044.0 332.0) (26042.0 416.0) (26038.0 487.0) (26024.0 539.0) (25974.0 602.0) (25962.0 604.0) ' \
                      '(25939.0 626.0) (25918.0 684.0) (25892.0 753.0) (25862.0 779.0) (25817.0 813.0) (25748.0 834.0) ' \
                      '(25667.0 854.0) (25563.0 874.0) (25448.0 849.0) (25383.0 821.0) (25354.0 798.0) (25329.0 812.0) ' \
                      '(25285.0 826.0) (25192.0 799.0) (25163.0 793.0) (25040.0 796.0) (25003.0 785.0) (24932.0 803.0) ' \
                      '(24875.0 809.0) (24829.0 811.0) (24806.0 830.0) (24773.0 869.0) (24712.0 890.0) (24650.0 915.0) ' \
                      '(24494.0 969.0) (24435.0 967.0) (24406.0 954.0) (24388.0 996.0) (24358.0 1035.0) ' \
                      '(24319.0 1123.0) (24264.0 1161.0) (24244.0 1160.0) (24224.0 1177.0) (24127.0 1237.0) ' \
                      '(24078.0 1236.0) (24029.0 1257.0) (23940.0 1274.0) (23877.0 1253.0) (23779.0 1224.0) ' \
                      '(23665.0 1278.0) (23602.0 1486.0) (23583.0 1659.0) (23701.0 1750.0) (23685.0 2016.0) ' \
                      '(23678.0 2213.0) (23776.0 2536.0) (23819.0 2821.0) (24022.0 2860.0) (24149.0 2952.0) ' \
                      '(23734.0 3418.0))) (((26377.0 6035.0) (26350.0 6239.0) (29191.0 7378.0) (29416.0 7201.0) ' \
                      '(30336.0 6876.0) (30518.0 6640.0) (30538.0 5881.0) (30710.0 5841.0) (30409.0 5491.0) ' \
                      '(30321.0 5477.0) (30071.0 5506.0))) (((27219.0 5459.0) (27177.0 5693.0) (27351.0 5689.0) ' \
                      '(27366.0 5811.0) (30026.0 5423.0) (29786.0 5085.0) (29684.0 5021.0) (29698.0 4533.0) ' \
                      '(28444.0 5102.0) (28350.0 4915.0) (27845.0 5010.0) (27846.0 5057.0) (27239.0 5139.0))) ' \
                      '(((29740.0 4507.0) (29736.0 4992.0) (29860.0 5074.0) (30099.0 5425.0) (30828.0 5315.0) ' \
                      '(31156.0 5226.0) (31539.0 5013.0) (31815.0 4735.0) (32009.0 4397.0) (31436.0 3913.0) ' \
                      '(31316.0 4115.0) (31362.0 4252.0) (31455.0 4365.0) (31690.0 4380.0) (31778.0 4648.0) ' \
                      '(31510.0 4746.0) (31437.0 4724.0) (31278.0 4820.0) (30808.0 4809.0) (30612.0 4760.0) ' \
                      '(30096.0 4719.0) (29979.0 4588.0) (29896.0 4651.0) (29781.0 4456.0))) (((31063.0 5325.0) ' \
                      '(30388.0 5439.0) (30454.0 5470.0) (30786.0 5845.0) (30987.0 5842.0) (31039.0 5954.0) ' \
                      '(31033.0 6413.0) (30880.0 6454.0) (30820.0 6928.0) (30593.0 7328.0) (30721.0 7328.0) ' \
                      '(30757.0 7250.0) (31427.0 6799.0) (31732.0 6764.0) (32053.0 6097.0) (32638.0 5437.0) ' \
                      '(32906.0 5391.0) (33015.0 5438.0) (33094.0 5422.0) (33105.0 5378.0) (33625.0 5244.0) ' \
                      '(33733.0 5281.0) (34041.0 5219.0) (34345.0 5105.0) (34444.0 4949.0) (34515.0 4956.0) ' \
                      '(34385.0 4632.0) (34543.0 4604.0) (34514.0 4462.0) (34137.0 4439.0) (33706.0 4062.0) ' \
                      '(33655.0 3506.0) (33184.0 3525.0) (32903.0 3583.0) (32625.0 3727.0) (32370.0 3929.0) ' \
                      '(32067.0 4413.0) (31881.0 4754.0) (31664.0 4996.0) (31405.0 5180.0))) (((32233.0 2655.0) ' \
                      '(32134.0 2986.0) (32357.0 3021.0) (32380.0 3199.0) (32155.0 3243.0) (32284.0 3833.0) ' \
                      '(32434.0 3715.0) (32396.0 3330.0) (32655.0 3289.0) (32730.0 3547.0) (33240.0 3435.0) ' \
                      '(33196.0 3128.0) (32824.0 3196.0) (32801.0 3001.0))))'

        resp_object = self.cursor.execute_simple_query('koepenick')
        result = spatial.convert_region_to_list_exp_str(resp_object)
        self.assertIsInstance(result, str)
        self.assertEqual(result, comp_string)
