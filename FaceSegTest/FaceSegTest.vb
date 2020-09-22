Imports OfficeOpenXml
Imports Emgu.CV
Imports System.ComponentModel
Imports System.Runtime.InteropServices
Imports System.Runtime.Serialization
Imports System.Windows
Imports System.Windows.Media.Imaging
Imports System.Diagnostics

Module FaceSegTest
    Private Const Resolution096 As Single = 96
    Private Const SelectionFileName As String = "Selections.dat"
    Private Const DefaultSave As String = "D:\Downloads\"
    Private Const CaptureFolder As String = "CaptureVideo\"
    Private Const ClipFileName As String = "Clip.dat"
    Private Const ResultsFileName As String = "Results.xml"
    Private Const MainSegType As String = "FaceSeg"
    Private Const DataStr As String = "Data"
    Private Const TimeStr As String = "Time"
    Private Const DataFile As String = "FaceSegData.xlsx"
    Private Const NumStr As String = "No"
    Private Const ClipStr As String = "ClipType"
    Private Const SegStr As String = "Segmentation"
    Private Const DetectorStr As String = "Detector"
    Private Const TimeDetectStr As String = "PT-FaceDetect"
    Private Const TimeSetStr As String = "PT-Set"
    Private Const TimeGetStr As String = "PT-Get"
    Private Const CPUStr As String = "CPU Usage (%)"
    Private Const AccuracyAStr As String = "Accuracy10"
    Private Const AccuracyBStr As String = "Accuracy50"
    Private Const AccuracyCStr As String = "Accuracy90"
    Private Const F1AStr As String = "F1Score10"
    Private Const F1BStr As String = "F1Score50"
    Private Const F1CStr As String = "F1Score90"
    Private Const LowerResult As Single = 0.1 ' lower centile cutoff for result output
    Private Const MidResult As Single = 0.5 ' lower centile cutoff for result output
    Private Const UpperResult As Single = 0.9 ' lower centile cutoff for result output
    Private Const ProcessBegin As Integer = 20 ' begin analysis X seconds after start
    Private Const ProcessLength As Integer = 10 ' length of accuracy analysis
    Private Const FaceDetectInterval As Integer = 10 ' frame interval between face detection
    Private Const FPS As Integer = 30 ' fps of video clips
    Private Const SaveVideoWidth As Integer = 640
    Private Const SaveVideoHeight As Integer = 480
    Private Const topIndex As Int32 = 27
    Private Const bottomIndex As Int32 = 8
    Private Const landmarks As Int32 = 68
    Private ExcludedSegTypes As List(Of ClipData.SegTypeEnum) = {ClipData.SegTypeEnum.GMG, ClipData.SegTypeEnum.MixtureOfGaussianV1}.ToList
    Private NoDepthList As List(Of String) = {"GHO", "SLP", "DYN"}.ToList
    Private ProcessSegTypes As List(Of ClipData.SegTypeEnum) = {ClipData.SegTypeEnum.None, ClipData.SegTypeEnum.FastPortrait, ClipData.SegTypeEnum.LBAdaptiveSOM, ClipData.SegTypeEnum.LOBSTER, ClipData.SegTypeEnum.DPZivkovicAGMM, ClipData.SegTypeEnum.KDE}.ToList
    Private m_Processor As Processor
    Private m_GammaCorrect As GammaCorrect
    Private m_StructuringElement As Mat

    Sub Main()
        Using oCpuUsage As New PerformanceCounter("Processor", "% Processor Time", "_Total")
            System.Threading.Thread.Sleep(1000)
            oCpuUsage.NextValue()

            m_Processor = New Processor
            m_GammaCorrect = New GammaCorrect
            m_StructuringElement = CvInvoke.GetStructuringElement(CvEnum.ElementShape.Rectangle, New System.Drawing.Size(3, 3), New System.Drawing.Point(-1, -1))

#Region "Process"
            Dim sResultsFile As String = DefaultSave + CaptureFolder + ResultsFileName
            Dim oFaceSegResults As Dictionary(Of String, FaceSegResults) = Nothing
            If Not IO.File.Exists(sResultsFile) Then
                oFaceSegResults = New Dictionary(Of String, FaceSegResults)
            Else
                oFaceSegResults = DeserializeDataContractFile(Of Dictionary(Of String, FaceSegResults))(sResultsFile, FaceSegResults.GetKnownTypes, , , False)
            End If

            Dim sClipDatFile As String = DefaultSave + CaptureFolder + ClipFileName
            Dim oClipDataList As List(Of ClipData) = DeserializeDataContractFile(Of List(Of ClipData))(sClipDatFile)
            For Each oClipData In oClipDataList
                For Each oSegType In ProcessSegTypes
                    If Not ExcludedSegTypes.Contains(oSegType) Then
                        ProcessClipData(oSegType, oClipData, oFaceSegResults, oCpuUsage)
                    End If
                Next
            Next

            If IO.File.Exists(sResultsFile) Then
                IO.File.Delete(sResultsFile)
            End If
            SerializeDataContractFile(sResultsFile, oFaceSegResults, FaceSegResults.GetKnownTypes, , , False)
#End Region

            ' export results to Excel
#Region "Report"
            ExcelPackage.LicenseContext = OfficeOpenXml.LicenseContext.NonCommercial
            Using oDataDocument As New ExcelPackage()
                oDataDocument.Workbook.Worksheets.Add(DataStr)
                oDataDocument.Workbook.Worksheets.Add(TimeStr)
                Using oDataSheet As ExcelWorksheet = oDataDocument.Workbook.Worksheets(DataStr)
                    Using oTimeSheet As ExcelWorksheet = oDataDocument.Workbook.Worksheets(TimeStr)
                        ' DATA SHEET
                        Const iNumColData As Integer = 1
                        Const iClipColData As Integer = 2
                        Const iSegDetectorColData As Integer = 3
                        Const iTimeDetectColData As Integer = 4
                        Const iTimeSetColData As Integer = 5
                        Const iTimeGetColData As Integer = 6
                        Const iCPUData As Integer = 7
                        Const iAccuracyAColData As Integer = 8
                        Const iAccuracyBColData As Integer = 9
                        Const iAccuracyCColData As Integer = 10
                        Const iF1AColData As Integer = 11
                        Const iF1BColData As Integer = 12
                        Const iF1CColData As Integer = 13

                        oDataSheet.SetValue(1, iNumColData, NumStr)
                        oDataSheet.SetValue(1, iClipColData, ClipStr)
                        oDataSheet.SetValue(1, iSegDetectorColData, SegStr)
                        oDataSheet.SetValue(1, iTimeDetectColData, TimeDetectStr)
                        oDataSheet.SetValue(1, iTimeSetColData, TimeSetStr)
                        oDataSheet.SetValue(1, iTimeGetColData, TimeGetStr)
                        oDataSheet.SetValue(1, iCPUData, CPUStr)
                        oDataSheet.SetValue(1, iAccuracyAColData, AccuracyAStr)
                        oDataSheet.SetValue(1, iAccuracyBColData, AccuracyBStr)
                        oDataSheet.SetValue(1, iAccuracyCColData, AccuracyCStr)
                        oDataSheet.SetValue(1, iF1AColData, F1AStr)
                        oDataSheet.SetValue(1, iF1BColData, F1BStr)
                        oDataSheet.SetValue(1, iF1CColData, F1CStr)

                        Dim oFuncAccuracy As Func(Of Tuple(Of FaceSegResults.SplitResults, String), Tuple(Of Single, Single, Single)) = Function(ByVal oResultTuple As Tuple(Of FaceSegResults.SplitResults, String)) As Tuple(Of Single, Single, Single)
                                                                                                                                            If NoDepthList.Contains(oResultTuple.Item2) Then
                                                                                                                                                Return New Tuple(Of Single, Single, Single)(-1, -1, -1)
                                                                                                                                            Else
                                                                                                                                                Dim oResults As FaceSegResults.SplitResults = oResultTuple.Item1

                                                                                                                                                Dim oAccuracyList As New List(Of Double)
                                                                                                                                                For Each oResult In oResults.Results
                                                                                                                                                    Dim fAccuracy As Double = CDbl(oResult.TP + oResult.TN) / CDbl(oResult.TP + oResult.TN + oResult.FP + oResult.FN)
                                                                                                                                                    If Not Double.IsNaN(fAccuracy) Then
                                                                                                                                                        oAccuracyList.Add(fAccuracy)
                                                                                                                                                    End If
                                                                                                                                                Next

                                                                                                                                                oAccuracyList = oAccuracyList.OrderBy(Function(x) x).ToList
                                                                                                                                                Dim fC1 As Single = oAccuracyList((oAccuracyList.Count - 1) * LowerResult)
                                                                                                                                                Dim fC2 As Single = oAccuracyList((oAccuracyList.Count - 1) * MidResult)
                                                                                                                                                Dim fC3 As Single = oAccuracyList((oAccuracyList.Count - 1) * UpperResult)

                                                                                                                                                Return New Tuple(Of Single, Single, Single)(fC1, fC2, fC3)
                                                                                                                                            End If
                                                                                                                                        End Function
                        Dim oFuncF1Score As Func(Of Tuple(Of FaceSegResults.SplitResults, String), Tuple(Of Single, Single, Single)) = Function(ByVal oResultTuple As Tuple(Of FaceSegResults.SplitResults, String)) As Tuple(Of Single, Single, Single)
                                                                                                                                           If NoDepthList.Contains(oResultTuple.Item2) Then
                                                                                                                                               Return New Tuple(Of Single, Single, Single)(-1, -1, -1)
                                                                                                                                           Else
                                                                                                                                               Dim oResults As FaceSegResults.SplitResults = oResultTuple.Item1

                                                                                                                                               Dim oF1List As New List(Of Double)
                                                                                                                                               For Each oResult In oResults.Results
                                                                                                                                                   Dim fPrecision As Double = CDbl(oResult.TP) / CDbl(oResult.TP + oResult.FP)
                                                                                                                                                   Dim fRecall As Double = CDbl(oResult.TP) / CDbl(oResult.TP + oResult.FN)
                                                                                                                                                   Dim fF1 As Double = 2.0 * (fPrecision * fRecall) / (fPrecision + fRecall)
                                                                                                                                                   If Not Double.IsNaN(fF1) Then
                                                                                                                                                       oF1List.Add(fF1)
                                                                                                                                                   End If
                                                                                                                                               Next

                                                                                                                                               oF1List = oF1List.OrderBy(Function(x) x).ToList
                                                                                                                                               Dim fC1 As Single = oF1List((oF1List.Count - 1) * LowerResult)
                                                                                                                                               Dim fC2 As Single = oF1List((oF1List.Count - 1) * MidResult)
                                                                                                                                               Dim fC3 As Single = oF1List((oF1List.Count - 1) * UpperResult)

                                                                                                                                               Return New Tuple(Of Single, Single, Single)(fC1, fC2, fC3)
                                                                                                                                           End If
                                                                                                                                       End Function

                        Dim iCurrentRow As Integer = 2
                        For Each oClipData In oClipDataList
                            For Each oSegType In ProcessSegTypes
                                If (Not ExcludedSegTypes.Contains(oSegType)) AndAlso (Not NoDepthList.Contains(oClipData.Name)) Then
                                    For Each iInterval In If(oSegType = ClipData.SegTypeEnum.None, {1, FaceDetectInterval}, {1})
                                        Dim sSegTypeName As String = If(oSegType = ClipData.SegTypeEnum.None, MainSegType + "_" + iInterval.ToString, [Enum].GetName(GetType(ClipData.SegTypeEnum), oSegType))

                                        If oSegType = ClipData.SegTypeEnum.None Then
                                            For Each oFaceDetect In [Enum].GetValues(GetType(FaceDetect))
                                                Dim sSegDetectorName As String = MainSegType + If(oFaceDetect = FaceDetect.OpenFace, "OF(", "LFD(") + iInterval.ToString + ")"
                                                Dim oSplitResults As FaceSegResults.SplitResults = Nothing
                                                Select Case oFaceDetect
                                                    Case FaceDetect.OpenFace
                                                        oSplitResults = oFaceSegResults(oClipData.Name).OpenFaceResults(sSegTypeName)
                                                    Case FaceDetect.LibFacedetector
                                                        oSplitResults = oFaceSegResults(oClipData.Name).LibFacedetectorResults(sSegTypeName)
                                                    Case Else
                                                        oSplitResults = Nothing
                                                End Select

                                                If Not IsNothing(oSplitResults) Then
                                                    Dim oAccuracy As Tuple(Of Single, Single, Single) = oFuncAccuracy(New Tuple(Of FaceSegResults.SplitResults, String)(oSplitResults, oClipData.Name))
                                                    Dim oF1Score As Tuple(Of Single, Single, Single) = oFuncF1Score(New Tuple(Of FaceSegResults.SplitResults, String)(oSplitResults, oClipData.Name))

                                                    oDataSheet.SetValue(iCurrentRow, iNumColData, iCurrentRow - 1)
                                                    oDataSheet.SetValue(iCurrentRow, iClipColData, oClipData.Name)
                                                    oDataSheet.SetValue(iCurrentRow, iSegDetectorColData, sSegDetectorName)
                                                    oDataSheet.SetValue(iCurrentRow, iTimeDetectColData, oSplitResults.FaceDetectTime)
                                                    oDataSheet.SetValue(iCurrentRow, iTimeSetColData, oSplitResults.ProcessingTimeSet)
                                                    oDataSheet.SetValue(iCurrentRow, iTimeGetColData, oSplitResults.ProcessingTimeGet)
                                                    oDataSheet.SetValue(iCurrentRow, iCPUData, oSplitResults.ProcessingCPU)
                                                    oDataSheet.SetValue(iCurrentRow, iAccuracyAColData, oAccuracy.Item1)
                                                    oDataSheet.SetValue(iCurrentRow, iAccuracyBColData, oAccuracy.Item2)
                                                    oDataSheet.SetValue(iCurrentRow, iAccuracyCColData, oAccuracy.Item3)
                                                    oDataSheet.SetValue(iCurrentRow, iF1AColData, oF1Score.Item1)
                                                    oDataSheet.SetValue(iCurrentRow, iF1BColData, oF1Score.Item2)
                                                    oDataSheet.SetValue(iCurrentRow, iF1CColData, oF1Score.Item3)
                                                    iCurrentRow += 1
                                                End If
                                            Next
                                        Else
                                            Dim oSplitResults As FaceSegResults.SplitResults = oFaceSegResults(oClipData.Name).Results(sSegTypeName)

                                            Dim oAccuracy As Tuple(Of Single, Single, Single) = oFuncAccuracy(New Tuple(Of FaceSegResults.SplitResults, String)(oSplitResults, oClipData.Name))
                                            Dim oF1Score As Tuple(Of Single, Single, Single) = oFuncF1Score(New Tuple(Of FaceSegResults.SplitResults, String)(oSplitResults, oClipData.Name))

                                            oDataSheet.SetValue(iCurrentRow, iNumColData, iCurrentRow - 1)
                                            oDataSheet.SetValue(iCurrentRow, iClipColData, oClipData.Name)
                                            oDataSheet.SetValue(iCurrentRow, iSegDetectorColData, sSegTypeName)
                                            oDataSheet.SetValue(iCurrentRow, iTimeDetectColData, oSplitResults.FaceDetectTime)
                                            oDataSheet.SetValue(iCurrentRow, iTimeSetColData, oSplitResults.ProcessingTimeSet)
                                            oDataSheet.SetValue(iCurrentRow, iTimeGetColData, oSplitResults.ProcessingTimeGet)
                                            oDataSheet.SetValue(iCurrentRow, iCPUData, oSplitResults.ProcessingCPU)
                                            oDataSheet.SetValue(iCurrentRow, iAccuracyAColData, oAccuracy.Item1)
                                            oDataSheet.SetValue(iCurrentRow, iAccuracyBColData, oAccuracy.Item2)
                                            oDataSheet.SetValue(iCurrentRow, iAccuracyCColData, oAccuracy.Item3)
                                            oDataSheet.SetValue(iCurrentRow, iF1AColData, oF1Score.Item1)
                                            oDataSheet.SetValue(iCurrentRow, iF1BColData, oF1Score.Item2)
                                            oDataSheet.SetValue(iCurrentRow, iF1CColData, oF1Score.Item3)
                                            iCurrentRow += 1
                                        End If
                                    Next
                                End If
                            Next
                        Next

                        ' autofit columns
                        oDataSheet.Cells(oDataSheet.Dimension.Start.Row, oDataSheet.Dimension.Start.Column, oDataSheet.Dimension.End.Row, oDataSheet.Dimension.End.Column).AutoFitColumns()

                        ' TIME SHEET
                        Const iNumColTime As Integer = 1
                        Const iSegDetectorColTime As Integer = 2
                        Const iTimeSetColTime As Integer = 3
                        Const iTimeGetColTime As Integer = 4
                        Const iTimeDetectColTime As Integer = 5
                        Const iTimeCPUColTime As Integer = 6

                        oTimeSheet.SetValue(1, iNumColTime, NumStr)
                        oTimeSheet.SetValue(1, iSegDetectorColTime, SegStr)
                        oTimeSheet.SetValue(1, iTimeDetectColTime, TimeDetectStr)
                        oTimeSheet.SetValue(1, iTimeSetColTime, TimeSetStr)
                        oTimeSheet.SetValue(1, iTimeGetColTime, TimeGetStr)
                        oTimeSheet.SetValue(1, iTimeCPUColTime, CPUStr)

                        Dim oSegDetectorList As New List(Of Tuple(Of String, ClipData.SegTypeEnum, List(Of FaceDetect), Integer))
                        oSegDetectorList.Add(New Tuple(Of String, ClipData.SegTypeEnum, List(Of FaceDetect), Integer)(MainSegType + "OF(1)", ClipData.SegTypeEnum.None, {FaceDetect.OpenFace}.ToList, 1))
                        oSegDetectorList.Add(New Tuple(Of String, ClipData.SegTypeEnum, List(Of FaceDetect), Integer)(MainSegType + "OF(" + FaceDetectInterval.ToString + ")", ClipData.SegTypeEnum.None, {FaceDetect.OpenFace}.ToList, FaceDetectInterval))

                        oSegDetectorList.Add(New Tuple(Of String, ClipData.SegTypeEnum, List(Of FaceDetect), Integer)(MainSegType + "LFD(1)", ClipData.SegTypeEnum.None, {FaceDetect.LibFacedetector}.ToList, 1))
                        oSegDetectorList.Add(New Tuple(Of String, ClipData.SegTypeEnum, List(Of FaceDetect), Integer)(MainSegType + "LFD(" + FaceDetectInterval.ToString + ")", ClipData.SegTypeEnum.None, {FaceDetect.LibFacedetector}.ToList, FaceDetectInterval))

                        For Each oSegType In ProcessSegTypes
                            If (Not ExcludedSegTypes.Contains(oSegType)) AndAlso oSegType <> ClipData.SegTypeEnum.None Then
                                Dim sSegTypeName As String = [Enum].GetName(GetType(ClipData.SegTypeEnum), oSegType)
                                oSegDetectorList.Add(New Tuple(Of String, ClipData.SegTypeEnum, List(Of FaceDetect), Integer)(sSegTypeName, oSegType, {FaceDetect.None}.ToList, 1))
                            End If
                        Next

                        Dim oDetectDict As New Dictionary(Of Integer, Single)
                        Dim oSetDict As New Dictionary(Of Integer, Single)
                        Dim oGetDict As New Dictionary(Of Integer, Single)
                        Dim oCPUDict As New Dictionary(Of Integer, Single)
                        Dim oCountDict As New Dictionary(Of Integer, Integer)
                        For i = 0 To oSegDetectorList.Count - 1
                            oDetectDict.Add(i, 0)
                            oSetDict.Add(i, 0)
                            oGetDict.Add(i, 0)
                            oCPUDict.Add(i, 0)
                            oCountDict.Add(i, 0)
                        Next

                        For Each oClipData In oClipDataList
                            For Each oFaceDetect In [Enum].GetValues(GetType(FaceDetect))
                                For Each oSegType In ProcessSegTypes
                                    If (Not ExcludedSegTypes.Contains(oSegType)) AndAlso (Not NoDepthList.Contains(oClipData.Name)) Then
                                        For Each iInterval In If(oSegType = ClipData.SegTypeEnum.None, {1, FaceDetectInterval}, {1})
                                            For i = 0 To oSegDetectorList.Count - 1
                                                Dim sCombinedTypeName As String = If(oSegType = ClipData.SegTypeEnum.None, MainSegType + If(oFaceDetect = FaceDetect.OpenFace, "OF(", "LFD(") + iInterval.ToString + ")", [Enum].GetName(GetType(ClipData.SegTypeEnum), oSegType))
                                                If oSegDetectorList(i).Item1 = sCombinedTypeName AndAlso oSegDetectorList(i).Item2 = oSegType AndAlso oSegDetectorList(i).Item3.Contains(oFaceDetect) AndAlso oSegDetectorList(i).Item4 = iInterval Then
                                                    Dim sSegTypeName As String = If(oSegType = ClipData.SegTypeEnum.None, MainSegType + "_" + iInterval.ToString, [Enum].GetName(GetType(ClipData.SegTypeEnum), oSegType))
                                                    Dim oSplitResults As FaceSegResults.SplitResults = Nothing
                                                    Select Case oFaceDetect
                                                        Case FaceDetect.None
                                                            oSplitResults = oFaceSegResults(oClipData.Name).Results(sSegTypeName)
                                                        Case FaceDetect.OpenFace
                                                            oSplitResults = oFaceSegResults(oClipData.Name).OpenFaceResults(sSegTypeName)
                                                        Case FaceDetect.LibFacedetector
                                                            oSplitResults = oFaceSegResults(oClipData.Name).LibFacedetectorResults(sSegTypeName)
                                                    End Select

                                                    Dim fDetect As Single = 0
                                                    Dim fSet As Single = 0
                                                    Dim fGet As Single = 0
                                                    Dim fCPU As Single = 0

                                                    If oSegType = ClipData.SegTypeEnum.None Then
                                                        fDetect = oSplitResults.FaceDetectTime / CSng(iInterval)
                                                        fGet = oSplitResults.ProcessingTimeGet
                                                        fSet = oSplitResults.ProcessingTimeSet / CSng(iInterval)
                                                        fCPU = oSplitResults.ProcessingCPU
                                                    Else
                                                        fGet = oSplitResults.ProcessingTimeGet
                                                        fCPU = oSplitResults.ProcessingCPU
                                                    End If

                                                    oDetectDict(i) += fDetect
                                                    oGetDict(i) += fGet
                                                    oSetDict(i) += fSet
                                                    oCPUDict(i) += fCPU
                                                    oCountDict(i) += 1

                                                    Exit For
                                                End If
                                            Next
                                        Next
                                    End If
                                Next
                            Next
                        Next

                        iCurrentRow = 2
                        Const TimeFactor As Single = 1000.0
                        For i = 0 To oSegDetectorList.Count - 1
                            oTimeSheet.SetValue(iCurrentRow, iNumColTime, iCurrentRow - 1)
                            oTimeSheet.SetValue(iCurrentRow, iSegDetectorColTime, oSegDetectorList(i).Item1)
                            oTimeSheet.SetValue(iCurrentRow, iTimeSetColTime, oSetDict(i) / (CSng(oCountDict(i) * TimeFactor)))
                            oTimeSheet.SetValue(iCurrentRow, iTimeGetColTime, oGetDict(i) / (CSng(oCountDict(i) * TimeFactor)))
                            oTimeSheet.SetValue(iCurrentRow, iTimeDetectColTime, oDetectDict(i) / (CSng(oCountDict(i) * TimeFactor)))
                            oTimeSheet.SetValue(iCurrentRow, iTimeCPUColTime, oCPUDict(i) / (CSng(oCountDict(i))))
                            iCurrentRow += 1
                        Next

                        ' autofit columns
                        oTimeSheet.Cells(oTimeSheet.Dimension.Start.Row, oTimeSheet.Dimension.Start.Column, oTimeSheet.Dimension.End.Row, oTimeSheet.Dimension.End.Column).AutoFitColumns()

                        ' SAVE FILE
                        Dim sDataFile As String = DefaultSave + CaptureFolder + DataFile
                        Dim oDataInfo As New IO.FileInfo(sDataFile)
                        If oDataInfo.Exists Then
                            oDataInfo.Delete()
                        End If

                        oDataDocument.SaveAs(oDataInfo)
                        Console.WriteLine("Saved File " + oDataInfo.Name)
                    End Using
                End Using
            End Using
#End Region
        End Using
    End Sub
    Private Sub ProcessClipData(ByVal oSegType As ClipData.SegTypeEnum, ByVal oClipData As ClipData, ByRef oFaceSegResults As Dictionary(Of String, FaceSegResults), ByRef oCpuUsage As PerformanceCounter)
        Dim sDirectoryName As String = DefaultSave + CaptureFolder + oClipData.Name
        Dim oDirectoryInfo As New IO.DirectoryInfo(sDirectoryName)
        Dim oFileInfoList As List(Of IO.FileInfo) = oDirectoryInfo.EnumerateFiles("[??????]_" + SafeFileName(oClipData.Name) + "_GroundTruth_Cut.avi").ToList
        If oFileInfoList.Count > 0 OrElse oClipData.IgnoreDepth Then
            Dim sGroundTruthVideoFileName As String = String.Empty
            If Not oClipData.IgnoreDepth Then
                Dim oFileInfo As IO.FileInfo = oFileInfoList.First
                sGroundTruthVideoFileName = oFileInfo.FullName
            End If

            oFileInfoList = oDirectoryInfo.EnumerateFiles("[??????]_" + SafeFileName(oClipData.Name) + "_Colour_Cut.avi").ToList
            If oFileInfoList.Count > 0 Then
                Dim oFileInfo As IO.FileInfo = oFileInfoList.First
                Dim sColourVideoFileName As String = oFileInfo.FullName

                If oSegType = ClipData.SegTypeEnum.None Then
                    ProcessClipDetect(oSegType, 1, FaceDetect.OpenFace, oClipData, oFaceSegResults, oCpuUsage, oFileInfo, sGroundTruthVideoFileName, sColourVideoFileName)
                    ProcessClipDetect(oSegType, 1, FaceDetect.LibFacedetector, oClipData, oFaceSegResults, oCpuUsage, oFileInfo, sGroundTruthVideoFileName, sColourVideoFileName)
                    ProcessClipDetect(oSegType, FaceDetectInterval, FaceDetect.OpenFace, oClipData, oFaceSegResults, oCpuUsage, oFileInfo, sGroundTruthVideoFileName, sColourVideoFileName)
                    ProcessClipDetect(oSegType, FaceDetectInterval, FaceDetect.LibFacedetector, oClipData, oFaceSegResults, oCpuUsage, oFileInfo, sGroundTruthVideoFileName, sColourVideoFileName)
                Else
                    ProcessClipDetect(oSegType, 1, FaceDetect.None, oClipData, oFaceSegResults, oCpuUsage, oFileInfo, sGroundTruthVideoFileName, sColourVideoFileName)
                End If
            End If
        End If
    End Sub
    Private Sub ProcessClipDetect(ByVal oSegType As ClipData.SegTypeEnum, ByVal iInterval As Integer, ByVal oFaceDetect As FaceDetect, ByVal oClipData As ClipData, ByRef oFaceSegResults As Dictionary(Of String, FaceSegResults), ByRef oCpuUsage As PerformanceCounter, ByVal oFileInfo As IO.FileInfo, ByVal sGroundTruthVideoFileName As String, ByVal sColourVideoFileName As String)
        If Not oClipData.IgnoreDepth Then
            Dim sSegTypeName As String = If(oSegType = ClipData.SegTypeEnum.None, MainSegType + "_" + iInterval.ToString, [Enum].GetName(GetType(ClipData.SegTypeEnum), oSegType))
            Dim sFaceDetectName As String = [Enum].GetName(GetType(FaceDetect), oFaceDetect)
            Dim sFaceColourSegmentedVideoFileName As String = Left(oFileInfo.FullName, Len(oFileInfo.FullName) - Len("_Colour_Cut.avi")) + "_FaceColourSegmented_" + sSegTypeName + "_" + sFaceDetectName + "_Cut.avi"
            Dim sGTColourSegmentedVideoFileName As String = Left(oFileInfo.FullName, Len(oFileInfo.FullName) - Len("_Colour_Cut.avi")) + "_FaceColourGroundTruth_" + sSegTypeName + "_" + sFaceDetectName + "_Cut.avi"

            Dim iProcessStartFrame As Integer = ProcessBegin * FPS
            Dim iProcessEndFrame As Integer = (ProcessBegin + ProcessLength) * FPS

            If Not (IO.File.Exists(sFaceColourSegmentedVideoFileName) AndAlso IO.File.Exists(sGTColourSegmentedVideoFileName)) Then
                If IO.File.Exists(sFaceColourSegmentedVideoFileName) Then
                    IO.File.Delete(sFaceColourSegmentedVideoFileName)
                End If
                If IO.File.Exists(sGTColourSegmentedVideoFileName) Then
                    IO.File.Delete(sGTColourSegmentedVideoFileName)
                End If

                Using oGroundTruthVideoReader As New Accord.Video.FFMPEG.VideoFileReader
                    oGroundTruthVideoReader.Open(sGroundTruthVideoFileName)

                    Using oColourVideoReader As New Accord.Video.FFMPEG.VideoFileReader
                        oColourVideoReader.Open(sColourVideoFileName)

                        If iProcessEndFrame <= oColourVideoReader.FrameCount - 1 Then
                            Using oFaceColourSegmentedVideoWriter As New Accord.Video.FFMPEG.VideoFileWriter
                                oFaceColourSegmentedVideoWriter.Open(sFaceColourSegmentedVideoFileName, oColourVideoReader.Width, oColourVideoReader.Height, New Accord.Math.Rational(FPS), Accord.Video.FFMPEG.VideoCodec.MPEG4, 8000000)
                                Using oGTColourSegmentedVideoWriter As New Accord.Video.FFMPEG.VideoFileWriter
                                    oGTColourSegmentedVideoWriter.Open(sGTColourSegmentedVideoFileName, oColourVideoReader.Width, oColourVideoReader.Height, New Accord.Math.Rational(FPS), Accord.Video.FFMPEG.VideoCodec.MPEG4, 8000000)

                                    Dim bValid As Boolean = True
                                    initSegmenter(topIndex, bottomIndex, landmarks)
                                    Select Case oSegType
                                        Case ClipData.SegTypeEnum.None
                                        Case ClipData.SegTypeEnum.FastPortrait
                                            initFP(oColourVideoReader.Width, oColourVideoReader.Height)
                                        Case Else
                                            initSegmenter(0, oSegType)
                                            bValid = validbgs(0)
                                    End Select

                                    If bValid Then
                                        Dim iProcessCount As Long = 0
                                        Dim iSetProcessCount As Long = 0
                                        Dim iGetProcessCount As Long = 0
                                        Dim iTotalFaceDetectTime As Long = 0
                                        Dim fTotalCPUTime As Single = 0
                                        Dim iTotalProcessingTimeSet As Long = 0
                                        Dim iTotalProcessingTimeGet As Long = 0
                                        Dim oSplitResults As New FaceSegResults.SplitResults

                                        Dim iFaceDetectSuccess As Integer = 0
                                        For i = 0 To oColourVideoReader.FrameCount - 1
                                            Using oGroundTruthBitmap As System.Drawing.Bitmap = oGroundTruthVideoReader.ReadVideoFrame(i)
                                                Using oColourBitmap As System.Drawing.Bitmap = oColourVideoReader.ReadVideoFrame(i)
                                                    Using oGroundTruthMatrix As Matrix(Of Byte) = BitmapToMatrix(oGroundTruthBitmap)
                                                        Using oMonoGroundTruthMatrix As New Matrix(Of Byte)(oGroundTruthMatrix.Size)
                                                            CvInvoke.CvtColor(oGroundTruthMatrix, oMonoGroundTruthMatrix, CvEnum.ColorConversion.Bgr2Gray)

                                                            Using oColourMatrix As Matrix(Of Byte) = BitmapToMatrix(oColourBitmap)
                                                                Using oGammaMatrix As Matrix(Of Byte) = m_GammaCorrect.CorrectGamma(oColourMatrix)
                                                                    If oSegType = ClipData.SegTypeEnum.None Then
                                                                        Dim oTimeStart As Date = Date.Now
                                                                        Dim oResults As List(Of FaceResult) = If(oFaceDetect = FaceDetect.OpenFace, m_Processor.DetectOpenFace(oGammaMatrix, 1.0), m_Processor.DetectLibfacedetector(oGammaMatrix, 1.0))
                                                                        Dim oTimeEnd As Date = Date.Now
                                                                        Dim iFaceDetectDuration As Integer = (oTimeEnd - oTimeStart).TotalMilliseconds * 1000

                                                                        If oResults.Count > 0 Then
                                                                            ' only run detection at regular intervals
                                                                            If iFaceDetectSuccess Mod iInterval = 0 Then
                                                                                Dim iSetDuration As Integer = SetSegment(oResults, oGammaMatrix)
                                                                                If i >= iProcessStartFrame AndAlso i <= iProcessEndFrame Then
                                                                                    iSetProcessCount += 1
                                                                                    iTotalProcessingTimeSet += (iSetDuration)
                                                                                End If
                                                                            End If
                                                                            iFaceDetectSuccess += 1
                                                                        End If

                                                                        Dim oMask As Matrix(Of Byte) = Nothing
                                                                        Dim iGetDuration As Integer = GetSegment(oGammaMatrix, oMask)

                                                                        If i >= iProcessStartFrame AndAlso i <= iProcessEndFrame Then
                                                                            iGetProcessCount += 1
                                                                            iTotalProcessingTimeGet += (iGetDuration)
                                                                            fTotalCPUTime += oCpuUsage.NextValue

                                                                            Dim oSplitResult As FaceSegResults.SplitResults.SplitResult = ProcessResult(oMask, oMonoGroundTruthMatrix)
                                                                            If Not IsNothing(oSplitResult) Then
                                                                                iProcessCount += 1
                                                                                oSplitResults.Results.Add(oSplitResult)
                                                                                iTotalFaceDetectTime += iFaceDetectDuration
                                                                            End If

                                                                            SaveMask(i, oGammaMatrix, oMask, oColourBitmap, oFaceColourSegmentedVideoWriter)
                                                                            SaveMask(i, oGammaMatrix, oMonoGroundTruthMatrix, oColourBitmap, oGTColourSegmentedVideoWriter)
                                                                        Else
                                                                            SaveMask(i, oGammaMatrix, Nothing, oColourBitmap, oFaceColourSegmentedVideoWriter)
                                                                            SaveMask(i, oGammaMatrix, Nothing, oColourBitmap, oGTColourSegmentedVideoWriter)
                                                                        End If

                                                                        If MatrixNotNothing(oMask) Then
                                                                            oMask.Dispose()
                                                                            oMask = Nothing
                                                                        End If
                                                                    Else
                                                                        Using oColourMask As Matrix(Of Byte) = Segment(oSegType, 0, oGammaMatrix, iTotalProcessingTimeGet)
                                                                            If i >= iProcessStartFrame AndAlso i <= iProcessEndFrame Then
                                                                                iGetProcessCount += 1
                                                                                fTotalCPUTime += oCpuUsage.NextValue

                                                                                Using oMask As New Matrix(Of Byte)(oColourMask.Size)
                                                                                    CvInvoke.CvtColor(oColourMask, oMask, CvEnum.ColorConversion.Bgr2Gray)

                                                                                    Dim oSplitResult As FaceSegResults.SplitResults.SplitResult = ProcessResult(oMask, oMonoGroundTruthMatrix)
                                                                                    If Not IsNothing(oSplitResult) Then
                                                                                        iProcessCount += 1
                                                                                        oSplitResults.Results.Add(oSplitResult)
                                                                                    End If

                                                                                    SaveMask(i, oGammaMatrix, oMask, oColourBitmap, oFaceColourSegmentedVideoWriter)
                                                                                    SaveMask(i, oGammaMatrix, oMonoGroundTruthMatrix, oColourBitmap, oGTColourSegmentedVideoWriter)
                                                                                End Using
                                                                            Else
                                                                                SaveMask(i, oGammaMatrix, Nothing, oColourBitmap, oFaceColourSegmentedVideoWriter)
                                                                                SaveMask(i, oGammaMatrix, Nothing, oColourBitmap, oGTColourSegmentedVideoWriter)
                                                                            End If
                                                                        End Using
                                                                    End If
                                                                End Using
                                                            End Using
                                                        End Using
                                                    End Using
                                                End Using
                                            End Using
                                        Next

                                        If iProcessCount > 0 Then
                                            With oSplitResults
                                                .FaceDetectTime = iTotalFaceDetectTime / CSng(iProcessCount)
                                                .ProcessingCPU = If(iGetProcessCount = 0, -1, fTotalCPUTime / CSng(iGetProcessCount))
                                                .ProcessingTimeSet = If(iSetProcessCount = 0, -1, CSng(iTotalProcessingTimeSet) / CSng(iSetProcessCount))
                                                .ProcessingTimeGet = If(iGetProcessCount = 0, -1, CSng(iTotalProcessingTimeGet) / CSng(iGetProcessCount))
                                            End With

                                            If Not oFaceSegResults.ContainsKey(oClipData.Name) Then
                                                oFaceSegResults.Add(oClipData.Name, New FaceSegResults)
                                            End If
                                            Select Case oFaceDetect
                                                Case FaceDetect.None
                                                    If oFaceSegResults(oClipData.Name).Results.ContainsKey(sSegTypeName) Then
                                                        oFaceSegResults(oClipData.Name).Results(sSegTypeName) = oSplitResults
                                                    Else
                                                        oFaceSegResults(oClipData.Name).Results.Add(sSegTypeName, oSplitResults)
                                                    End If
                                                Case FaceDetect.OpenFace
                                                    If oFaceSegResults(oClipData.Name).OpenFaceResults.ContainsKey(sSegTypeName) Then
                                                        oFaceSegResults(oClipData.Name).OpenFaceResults(sSegTypeName) = oSplitResults
                                                    Else
                                                        oFaceSegResults(oClipData.Name).OpenFaceResults.Add(sSegTypeName, oSplitResults)
                                                    End If
                                                Case FaceDetect.LibFacedetector
                                                    If oFaceSegResults(oClipData.Name).LibFacedetectorResults.ContainsKey(sSegTypeName) Then
                                                        oFaceSegResults(oClipData.Name).LibFacedetectorResults(sSegTypeName) = oSplitResults
                                                    Else
                                                        oFaceSegResults(oClipData.Name).LibFacedetectorResults.Add(sSegTypeName, oSplitResults)
                                                    End If
                                            End Select
                                        End If
                                    End If

                                    exitSegmenter()
                                    Select Case oSegType
                                        Case ClipData.SegTypeEnum.None
                                        Case ClipData.SegTypeEnum.FastPortrait
                                            exitFP()
                                        Case Else
                                            exitSegmenter(0)
                                    End Select
                                End Using
                            End Using
                        End If
                    End Using
                End Using
            End If
        End If
    End Sub
    Private Function SetSegment(ByVal oResults As List(Of FaceResult), ByVal oMatrixIn As Matrix(Of Byte)) As Integer
        Dim iDuration As Integer = 0
        Dim iFaceCount As Int32 = oResults.Count
        If iFaceCount > 0 Then
            Dim iPointFStructSize As Integer = Marshal.SizeOf(GetType(PointFStruct))
            Dim iPointsSize As Integer = iPointFStructSize * landmarks
            Dim iFacesSize As Integer = iPointsSize * iFaceCount
            Dim oPointsBufferIn(iFacesSize - 1) As Byte

            For i = 0 To iFaceCount - 1
                For j = 0 To landmarks - 1
                    Dim oPointFStructPointer As IntPtr = Marshal.AllocCoTaskMem(iPointFStructSize)
                    Marshal.StructureToPtr(New PointFStruct(oResults(i)(j)), oPointFStructPointer, False)
                    Marshal.Copy(oPointFStructPointer, oPointsBufferIn, (i * iPointsSize) + (j * iPointFStructSize), iPointFStructSize)
                    Marshal.FreeCoTaskMem(oPointFStructPointer)
                Next
            Next

            Dim oPointsBufferHandleIn As GCHandle = GCHandle.Alloc(oPointsBufferIn, GCHandleType.Pinned)

            Dim iMatStructSize As Integer = Marshal.SizeOf(GetType(MatStruct))
            Dim oMatStructIn As New MatStruct(oMatrixIn)
            Dim oMatPointerIn As IntPtr = Marshal.AllocCoTaskMem(iMatStructSize)
            Marshal.StructureToPtr(oMatStructIn, oMatPointerIn, False)

            Dim oMatBufferIn As Byte() = oMatrixIn.Bytes
            Dim oMatBufferHandleIn As GCHandle = GCHandle.Alloc(oMatBufferIn, GCHandleType.Pinned)

            setSegment(oPointsBufferHandleIn.AddrOfPinnedObject, iFaceCount, oMatPointerIn, oMatBufferHandleIn.AddrOfPinnedObject, iDuration)

            Marshal.FreeCoTaskMem(oMatPointerIn)
            oMatBufferHandleIn.Free()

            oPointsBufferHandleIn.Free()
        End If
        Return iDuration
    End Function
    Private Function GetSegment(ByVal oMatrixIn As Matrix(Of Byte), ByRef oMaskOut As Matrix(Of Byte)) As Integer
        Dim iDuration As Integer = 0

        Dim iMatStructSize As Integer = Marshal.SizeOf(GetType(MatStruct))
        Dim oMatStructIn As New MatStruct(oMatrixIn)
        Dim oMatPointerIn As IntPtr = Marshal.AllocCoTaskMem(iMatStructSize)
        Marshal.StructureToPtr(oMatStructIn, oMatPointerIn, False)

        Dim oMatBufferIn As Byte() = oMatrixIn.Bytes
        Dim oMatBufferHandleIn As GCHandle = GCHandle.Alloc(oMatBufferIn, GCHandleType.Pinned)

        oMaskOut = New Matrix(Of Byte)(oMatrixIn.Height, oMatrixIn.Width)
        Dim oMatStructOut As New MatStruct(oMaskOut)
        Dim oMatPointerOut As IntPtr = Marshal.AllocCoTaskMem(iMatStructSize)
        Marshal.StructureToPtr(oMatStructOut, oMatPointerOut, False)

        Dim oMatBufferOut As Byte() = oMaskOut.Bytes
        Dim oMatBufferHandleOut As GCHandle = GCHandle.Alloc(oMatBufferOut, GCHandleType.Pinned)

        Dim iFaceCount As Int32 = 0
        Dim iTrackID As Int32 = 0
        getSegment(iFaceCount, iTrackID, oMatPointerIn, oMatBufferHandleIn.AddrOfPinnedObject, oMatPointerOut, oMatBufferHandleOut.AddrOfPinnedObject, iDuration)

        Marshal.FreeCoTaskMem(oMatPointerIn)
        oMatBufferHandleIn.Free()

        Marshal.FreeCoTaskMem(oMatPointerOut)
        oMatBufferHandleOut.Free()

        ' copy back result to mask
        Using oReturnArray As Matrix(Of Byte) = oMatStructOut.GetMatrix(Of Byte)(oMatBufferOut)
            oReturnArray.Mat.CopyTo(oMaskOut)
        End Using

        If iFaceCount > 0 Then
            Dim iRect2fStructSize As Integer = Marshal.SizeOf(GetType(Rect2fStruct))
            Dim iRectsSize As Integer = iRect2fStructSize * iFaceCount
            Dim oRectsBufferOut(iRectsSize - 1) As Byte

            Dim oRectsBufferHandleOut As GCHandle = GCHandle.Alloc(oRectsBufferOut, GCHandleType.Pinned)

            getFetch(iTrackID, oRectsBufferHandleOut.AddrOfPinnedObject)

            oRectsBufferHandleOut.Free()

            For i = 0 To iFaceCount - 1
                Dim oRectStructPointer As IntPtr = Marshal.AllocCoTaskMem(iRect2fStructSize)
                Marshal.Copy(oRectsBufferOut, i * iRect2fStructSize, oRectStructPointer, iRect2fStructSize)
                Dim oRect As Rect2fStruct = Marshal.PtrToStructure(oRectStructPointer, GetType(Rect2fStruct))
                Marshal.FreeCoTaskMem(oRectStructPointer)
            Next
        End If

        Return iDuration
    End Function
    Private Function Segment(ByVal oSegType As ClipData.SegTypeEnum, ByVal iTask As Int32, ByVal oMatrix As Matrix(Of Byte), ByRef iTotalDuration As Long) As Matrix(Of Byte)
        ' segments image
        ' init segmenter first, then exit segmenter once video sequence is complete
        Dim iMatStructSize As Integer = Marshal.SizeOf(GetType(MatStruct))

        Dim oMatStructIn As New MatStruct(oMatrix)
        Dim oMatPointerIn As IntPtr = Marshal.AllocCoTaskMem(iMatStructSize)
        Marshal.StructureToPtr(oMatStructIn, oMatPointerIn, False)

        Dim oMatBufferIn As Byte() = oMatrix.Bytes
        Dim oBufferHandleIn As GCHandle = GCHandle.Alloc(oMatBufferIn, GCHandleType.Pinned)

        Dim oMask As New Matrix(Of Byte)(oMatrix.Height, oMatrix.Width, 3)

        Dim oMatStructOut As New MatStruct(oMask)
        Dim oMatPointerOut As IntPtr = Marshal.AllocCoTaskMem(iMatStructSize)
        Marshal.StructureToPtr(oMatStructOut, oMatPointerOut, False)

        Dim oMatBufferOut As Byte() = oMask.Bytes
        Dim oBufferHandleOut As GCHandle = GCHandle.Alloc(oMatBufferOut, GCHandleType.Pinned)

        Dim iDuration As Integer = 0
        Dim bSuccess As Boolean = False
        Select Case oSegType
            Case ClipData.SegTypeEnum.None
            Case ClipData.SegTypeEnum.FastPortrait
                bSuccess = segmentFP(oMatPointerIn, oBufferHandleIn.AddrOfPinnedObject, oMatPointerOut, oBufferHandleOut.AddrOfPinnedObject, iDuration)
            Case Else
                bSuccess = segment(iTask, oMatPointerIn, oBufferHandleIn.AddrOfPinnedObject, oMatPointerOut, oBufferHandleOut.AddrOfPinnedObject, iDuration)
        End Select
        If bSuccess Then
            iTotalDuration += iDuration
        End If

        Marshal.FreeCoTaskMem(oMatPointerIn)
        oBufferHandleIn.Free()

        Marshal.FreeCoTaskMem(oMatPointerOut)
        oBufferHandleOut.Free()

        Using oReturnArray As Matrix(Of Byte) = oMatStructOut.GetMatrix(Of Byte)(oMatBufferOut)
            oReturnArray.Mat.CopyTo(oMask)
        End Using

        Return oMask
    End Function
    Private Sub SaveMask(ByVal iFrameNumber As Integer, ByVal oGammaMatrix As Matrix(Of Byte), ByVal oMask As Matrix(Of Byte), ByVal oColourBitmap As System.Drawing.Bitmap, ByVal oColourSegmentedVideoWriter As Accord.Video.FFMPEG.VideoFileWriter)
        ' saves mask to video
        If MatrixNotNothing(oMask) Then
            Using oCombinedMatrix As Matrix(Of Byte) = oGammaMatrix.CopyBlank
                Using oColourMask As New Matrix(Of Byte)(oMask.Height, oMask.Width, 3)
                    CvInvoke.CvtColor(oMask, oColourMask, CvEnum.ColorConversion.Gray2Bgr)
                    CvInvoke.AddWeighted(oColourMask, 0.5, oGammaMatrix, 0.5, 0.0, oCombinedMatrix)
                    Using oColourSegmentedBitmap As System.Drawing.Bitmap = MatToBitmap(oCombinedMatrix.Mat, oColourBitmap.HorizontalResolution, oColourBitmap.VerticalResolution)
                        oColourSegmentedVideoWriter.WriteVideoFrame(oColourSegmentedBitmap, iFrameNumber)
                    End Using
                End Using
            End Using
        Else
            Using oColourSegmentedBitmap As System.Drawing.Bitmap = MatToBitmap(oGammaMatrix.Mat, oColourBitmap.HorizontalResolution, oColourBitmap.VerticalResolution)
                oColourSegmentedVideoWriter.WriteVideoFrame(oColourSegmentedBitmap, iFrameNumber)
            End Using
        End If
    End Sub
    Private Function ProcessResult(ByVal oMaskIn As Matrix(Of Byte), ByVal oGroundTruthIn As Matrix(Of Byte)) As FaceSegResults.SplitResults.SplitResult
        ' gets the number of pixels in the ground truth, the segmented pixels outside and inside of the ground truth
        If MatrixIsNothing(oMaskIn) OrElse MatrixIsNothing(oGroundTruthIn) Then
            Return Nothing
        Else
            Dim oSplitResult As New FaceSegResults.SplitResults.SplitResult
            Using oMask As Matrix(Of Byte) = oMaskIn.Clone
                Using oGroundTruth As Matrix(Of Byte) = oGroundTruthIn.Clone
                    Dim iPositiveCount As Integer = CvInvoke.CountNonZero(oMask)
                    Dim iNegativeCount As Integer = (oMaskIn.Width * oMaskIn.Height) - iPositiveCount

                    With oSplitResult
                        Using oCountMatrix As New Matrix(Of Byte)(oMask.Size)
                            ' true positive
                            CvInvoke.BitwiseAnd(oGroundTruth, oMask, oCountMatrix)
                            Dim iTruePositive = CvInvoke.CountNonZero(oCountMatrix)
                            .TP += iTruePositive

                            ' false positive
                            .FP += iPositiveCount - iTruePositive

                            ' invert matrices and mask
                            CvInvoke.BitwiseNot(oGroundTruth, oGroundTruth)
                            CvInvoke.BitwiseNot(oMask, oMask)

                            ' true negative
                            CvInvoke.BitwiseAnd(oGroundTruth, oMask, oCountMatrix)
                            Dim iTrueNegative = CvInvoke.CountNonZero(oCountMatrix)
                            .TN += iTrueNegative

                            ' false negative
                            .FN += iNegativeCount - iTrueNegative
                        End Using
                    End With
                End Using
            End Using
            Return oSplitResult
        End If
    End Function
    Private Class Processor
        Implements IDisposable

        Private m_FaceModelParameters As CppInterop.LandmarkDetector.FaceModelParameters
        Private m_FaceDetector As FaceDetectorInterop.FaceDetector
        Private m_CLNF As CppInterop.LandmarkDetector.CLNF

#Region "Constants"
        Private HalfFace As Integer() = {0, 16}
        Private Const FaceMargin As Single = 0.1
        Private Const LandmarkSize As Integer = 68
#End Region

        Sub New()
            m_FaceModelParameters = New CppInterop.LandmarkDetector.FaceModelParameters(AppDomain.CurrentDomain.BaseDirectory, True, False, False)
            m_FaceModelParameters.SetFaceDetector(False, False, True)
            m_FaceModelParameters.optimiseForImages()

            m_FaceDetector = New FaceDetectorInterop.FaceDetector(m_FaceModelParameters.GetHaarLocation(), m_FaceModelParameters.GetMTCNNLocation())
            m_CLNF = New CppInterop.LandmarkDetector.CLNF(m_FaceModelParameters)
        End Sub
        Public Function DetectOpenFace(ByVal oGammaMatrix As Matrix(Of Byte), ByVal fSmallScale As Single) As List(Of FaceResult)
            Dim oResults As New List(Of FaceResult)
            Using oSmallRawImage As OpenCVWrappers.RawImage = MatrixToRawImage(oGammaMatrix)
                Dim oSmallRegions As New List(Of Rect)

                Dim oConfidences As New List(Of Single)
                m_FaceDetector.DetectFacesMTCNN(oSmallRegions, oSmallRawImage, oConfidences)

                Dim oSmallFrameRect As New Rect(0, 0, oGammaMatrix.Width, oGammaMatrix.Height)
                For i = 0 To oSmallRegions.Count - 1
                    Dim bSuccess As Boolean = False

                    ' isolated the region together with a margin
                    Dim oRegionRect As Rect = oSmallRegions(i)
                    oRegionRect.Inflate(oRegionRect.Width * FaceMargin, oRegionRect.Height * FaceMargin)
                    Dim oIntersectRect As Rect = Rect.Intersect(oSmallFrameRect, oRegionRect)

                    Using oCropRawImage As OpenCVWrappers.RawImage = MatrixToRawImage(oGammaMatrix.GetSubRect(RectToRectangle(oIntersectRect)))
                        bSuccess = m_CLNF.DetectFaceLandmarksInImage(oCropRawImage, m_FaceModelParameters)
                        If bSuccess Then
                            Dim oReturnedLandmarks As List(Of Tuple(Of Single, Single)) = m_CLNF.CalculateAllLandmarks
                            Dim oLandmarks As List(Of System.Drawing.PointF) = (From oLandmark In oReturnedLandmarks Select New System.Drawing.PointF((oLandmark.Item1 + oIntersectRect.X) / fSmallScale, (oLandmark.Item2 + oIntersectRect.Y) / fSmallScale)).ToList
                            Dim oFaceRect As [Structure].RotatedRect = CvInvoke.MinAreaRect(New Util.VectorOfPointF(oLandmarks.ToArray))

                            Dim oFaceResult As New FaceResult(oFaceRect, oLandmarks)

                            oFaceResult.ResetRect()
                            oResults.Add(oFaceResult)
                        End If
                    End Using

                    ' reset for next face
                    m_CLNF.Reset()
                Next
            End Using
            Return oResults
        End Function
        Public Function DetectLibfacedetector(ByVal oGammaMatrix As Matrix(Of Byte), ByVal fSmallScale As Double) As List(Of FaceResult)
            ' face detector using libfacedetector
            Dim oResults As New List(Of FaceResult)
            Using oMonoMatrix As New Matrix(Of Byte)(oGammaMatrix.Size)
                CvInvoke.CvtColor(oGammaMatrix, oMonoMatrix, CvEnum.ColorConversion.Bgr2Gray)

                Dim iRotatedRectStructSize As Integer = Marshal.SizeOf(GetType(RotatedRectStruct))
                Dim iPointFStructSize As Integer = Marshal.SizeOf(GetType(PointFStruct))
                Dim iFaceResultsSize As Integer = iRotatedRectStructSize + (iPointFStructSize * LandmarkSize)
                Dim iMatStructSize As Integer = Marshal.SizeOf(GetType(MatStruct))

                Dim oMatStructIn As New MatStruct(oMonoMatrix)
                Dim oMatPointerIn As IntPtr = Marshal.AllocCoTaskMem(iMatStructSize)
                Marshal.StructureToPtr(oMatStructIn, oMatPointerIn, False)

                Dim oMatBufferIn As Byte() = oMonoMatrix.Bytes
                Dim oBufferHandleIn As GCHandle = GCHandle.Alloc(oMatBufferIn, GCHandleType.Pinned)

                Dim iFaceCount As Int32 = 0

                FD_multiview_reinforce(oMatPointerIn, oBufferHandleIn.AddrOfPinnedObject, IntPtr.Zero, iFaceCount)

                Dim oFaceResultsBufferOut((iFaceResultsSize * iFaceCount) - 1) As Byte
                Dim oFaceResultsHandleOut As GCHandle = GCHandle.Alloc(oFaceResultsBufferOut, GCHandleType.Pinned)

                FD_multiview_reinforce(oMatPointerIn, IntPtr.Zero, oFaceResultsHandleOut.AddrOfPinnedObject, iFaceCount)

                oFaceResultsHandleOut.Free()

                For i = 0 To iFaceCount - 1
                    Dim oRotatedRectStructPointer As IntPtr = Marshal.AllocCoTaskMem(iRotatedRectStructSize)
                    Marshal.Copy(oFaceResultsBufferOut, i * iFaceResultsSize, oRotatedRectStructPointer, iRotatedRectStructSize)
                    Dim oOutRotatedRect As RotatedRectStruct = Marshal.PtrToStructure(oRotatedRectStructPointer, GetType(RotatedRectStruct))
                    Marshal.FreeCoTaskMem(oRotatedRectStructPointer)
                    Dim oRotatedRect As [Structure].RotatedRect = oOutRotatedRect.RotatedRect

                    Dim oLandmarks As New List(Of System.Drawing.PointF)
                    For j = 0 To LandmarkSize - 1
                        Dim oPointFStructPointer As IntPtr = Marshal.AllocCoTaskMem(iPointFStructSize)
                        Marshal.Copy(oFaceResultsBufferOut, (i * iFaceResultsSize) + iRotatedRectStructSize + (j * iPointFStructSize), oPointFStructPointer, iPointFStructSize)
                        Dim oOutPointF As PointFStruct = Marshal.PtrToStructure(oPointFStructPointer, GetType(PointFStruct))
                        Marshal.FreeCoTaskMem(oPointFStructPointer)
                        Dim oPoint As System.Drawing.PointF = oOutPointF.PointF
                        oLandmarks.Add(New System.Drawing.PointF(oPoint.X / fSmallScale, oPoint.Y / fSmallScale))
                    Next

                    oResults.Add(New FaceResult(New [Structure].RotatedRect(New System.Drawing.PointF(oRotatedRect.Center.X / fSmallScale, oRotatedRect.Center.Y / fSmallScale), New System.Drawing.SizeF(oRotatedRect.Size.Width / fSmallScale, oRotatedRect.Size.Height / fSmallScale), oRotatedRect.Angle), oLandmarks))
                Next

                Marshal.FreeCoTaskMem(oMatPointerIn)
                oBufferHandleIn.Free()
            End Using

            Return oResults
        End Function
        Private Shared Function MatrixToRawImage(ByVal oMatrix As Matrix(Of Byte)) As OpenCVWrappers.RawImage
            ' converts a matrix to the rawimage format
            Return New OpenCVWrappers.RawImage(oMatrix.Mat)
        End Function
#Region "IDisposable Support"
        Private disposedValue As Boolean
        Protected Overridable Sub Dispose(disposing As Boolean)
            If Not disposedValue Then
                If disposing Then
                    ' TODO: dispose managed state (managed objects).
                    m_FaceModelParameters.Dispose()
                    m_FaceDetector.Dispose()
                    m_CLNF.Dispose()
                End If

                ' TODO: free unmanaged resources (unmanaged objects) and override Finalize() below.
                ' TODO: set large fields to null.
            End If
            disposedValue = True
        End Sub
        ' TODO: override Finalize() only if Dispose(disposing As Boolean) above has code to free unmanaged resources.
        'Protected Overrides Sub Finalize()
        '    ' Do not change this code.  Put cleanup code in Dispose(disposing As Boolean) above.
        '    Dispose(False)
        '    MyBase.Finalize()
        'End Sub
        Public Sub Dispose() Implements IDisposable.Dispose
            Dispose(True)
            ' TODO: uncomment the following line if Finalize() is overridden above.
            ' GC.SuppressFinalize(Me)
        End Sub
#End Region
    End Class
    Private Class GammaCorrect
        Implements IDisposable

        Private Const GammaLow As Single = 0.1 ' gamma search range
        Private Const GammaHigh As Single = 5.0 ' gamma search range
        Private Const GammaStep As Single = 0.05 ' gamma search range
        Private Const TargetColourBrightness As Byte = 160 ' target brightness value for the colour images
        Private Const GammaSmallSize As Integer = 10 ' processing size to determine image brighness
        Private Const GammaRangeRestriction As Integer = 8 ' restricts brightness search to remove black and white pixels
        Private m_GammaLutDict As Dictionary(Of Integer, Matrix(Of Byte))

        Sub New()
            m_GammaLutDict = New Dictionary(Of Integer, Matrix(Of Byte))
            Using oLUTBase As New Matrix(Of Byte)(256, 1)
                For i = 0 To 255
                    oLUTBase(i, 0) = i
                Next

                For i = GammaLow To GammaHigh Step GammaStep
                    Using oLUTBitmap As System.Drawing.Bitmap = MatToBitmap(oLUTBase.Mat, 300)
                        Dim oLUTGammaCorrection As New Accord.Imaging.Filters.GammaCorrection(i)
                        oLUTGammaCorrection.ApplyInPlace(oLUTBitmap)
                        Dim oLUTTarget As Matrix(Of Byte) = BitmapToMatrix(oLUTBitmap)

                        Dim iTrim As Integer = i * 100
                        m_GammaLutDict.Add(iTrim, oLUTTarget)
                    End Using
                Next
            End Using
        End Sub
        Public Function CorrectGamma(ByVal oMatrix As Matrix(Of Byte)) As Matrix(Of Byte)
            ' correct gamma to target
            Dim iGamma As Integer = 100
            Dim oReturnMatrix As Matrix(Of Byte) = oMatrix.Clone
            Using oMonoMatrix As New Matrix(Of Byte)(oMatrix.Size)
                CvInvoke.CvtColor(oMatrix, oMonoMatrix, CvEnum.ColorConversion.Bgr2Gray)
                Using oSmallMatrix As New Matrix(Of Byte)(GammaSmallSize, GammaSmallSize)
                    CvInvoke.Resize(oMonoMatrix, oSmallMatrix, oSmallMatrix.Size)
                    Using oRangeMask As New Matrix(Of Byte)(oSmallMatrix.Size)
                        Using oUpperRangeMask As New Matrix(Of Byte)(oSmallMatrix.Size)
                            oRangeMask.SetValue(GammaRangeRestriction)
                            oUpperRangeMask.SetValue(245 - GammaRangeRestriction)
                            CvInvoke.InRange(oSmallMatrix, oRangeMask, oUpperRangeMask, oRangeMask)

                            Dim fBrightness As Double = CvInvoke.Mean(oSmallMatrix, oRangeMask).V0

                            ' get list of gamma values and corrected brightness
                            For i = GammaLow To GammaHigh Step GammaStep
                                Using oTestMatrix As New Matrix(Of Byte)(2, 2)
                                    oTestMatrix.SetValue(fBrightness)

                                    Dim iTrim As Integer = i * 100
                                    CvInvoke.LUT(oTestMatrix, m_GammaLutDict(iTrim), oTestMatrix)
                                    Dim bCorrectedBrightness As Double = CvInvoke.Mean(oTestMatrix).V0
                                    If bCorrectedBrightness >= TargetColourBrightness Then
                                        iGamma = iTrim
                                        Exit For
                                    End If
                                End Using
                            Next
                        End Using
                    End Using
                End Using
            End Using

            If iGamma <> 100 Then
                CvInvoke.LUT(oReturnMatrix, m_GammaLutDict(iGamma), oReturnMatrix)
            End If
            Return oReturnMatrix
        End Function
#Region "IDisposable Support"
        Private disposedValue As Boolean
        Protected Overridable Sub Dispose(disposing As Boolean)
            If Not disposedValue Then
                If disposing Then
                    ' TODO: dispose managed state (managed objects).
                    For Each fKey In m_GammaLutDict.Keys
                        If MatrixNotNothing(m_GammaLutDict(fKey)) Then
                            m_GammaLutDict(fKey).Dispose()
                            m_GammaLutDict(fKey) = Nothing
                        End If
                    Next
                End If

                ' TODO: free unmanaged resources (unmanaged objects) and override Finalize() below.
                ' TODO: set large fields to null.
            End If
            disposedValue = True
        End Sub
        ' TODO: override Finalize() only if Dispose(disposing As Boolean) above has code to free unmanaged resources.
        'Protected Overrides Sub Finalize()
        '    ' Do not change this code.  Put cleanup code in Dispose(disposing As Boolean) above.
        '    Dispose(False)
        '    MyBase.Finalize()
        'End Sub
        Public Sub Dispose() Implements IDisposable.Dispose
            Dispose(True)
            ' TODO: uncomment the following line if Finalize() is overridden above.
            ' GC.SuppressFinalize(Me)
        End Sub
#End Region
    End Class
    Public Class FaceResult
        Inherits List(Of System.Drawing.PointF)

        Public Rect As [Structure].RotatedRect

        Sub New()
            Me.Clear()
            Rect = [Structure].RotatedRect.Empty
        End Sub
        Sub New(ByVal oRect As [Structure].RotatedRect, ByVal oPoints As List(Of System.Drawing.PointF))
            Me.Clear()
            Me.AddRange(oPoints)
            Rect = oRect
        End Sub
        Public Sub ResetRect()
            ' resets angle to be correct
            Dim fMidline As Single = GetMidlineAngle(GetApproxAngle, Rect)
            ResetRect(fMidline, Rect)
        End Sub
        Private Function GetApproxAngle() As Single
            ' returns approximate angle of the midline from the two points
            Dim oTopPoint As System.Drawing.PointF = Me(Midline.First)
            Dim oBottomPoint As System.Drawing.PointF = Me(Midline.Last)
            Dim fApproxAngle As Single = 0
            If oTopPoint.X <> oBottomPoint.X Then
                fApproxAngle = (((Math.Atan2(oTopPoint.Y - oBottomPoint.Y, oTopPoint.X - oBottomPoint.X) * 180 / Math.PI) + 90 + 360 + 180) Mod 360) - 180
            Else
                If oTopPoint.Y > oBottomPoint.Y Then
                    fApproxAngle = 180
                End If
            End If
            Return fApproxAngle
        End Function
        Private Function GetMidlineAngle(ByVal fMidline As Single, ByVal oRect As [Structure].RotatedRect) As Single
            ' gets the upright face angle of the rotated rect
            Dim oCompareAngles As New List(Of Tuple(Of Single, Single))
            For iAngle = -360 To 360 Step 90
                Dim fModifiedAngle As Single = oRect.Angle + iAngle
                Dim fDifference As Single = Math.Abs(fModifiedAngle - fMidline)
                oCompareAngles.Add(New Tuple(Of Single, Single)(fModifiedAngle, fDifference))
            Next

            Return (From oAngle In oCompareAngles Order By oAngle.Item2 Ascending Select oAngle.Item1).First
        End Function
        Private Sub ResetRect(ByVal fMidline As Single, ByRef oRect As [Structure].RotatedRect)
            ' resets the rotated rect using the approximate midline angle
            Dim oCompareAngles As New List(Of Tuple(Of Single, Single, Single))
            For iAngle = -360 To 360 Step 90
                Dim fModifiedAngle As Single = oRect.Angle + iAngle
                Dim fDifference As Single = Math.Abs(fModifiedAngle - fMidline)
                oCompareAngles.Add(New Tuple(Of Single, Single, Single)(fModifiedAngle, fDifference, (iAngle + 360) Mod 180))
            Next

            ' sort ascending according to difference
            oCompareAngles = oCompareAngles.OrderBy(Function(x) x.Item2).ToList

            oRect = New [Structure].RotatedRect(oRect.Center, If(oCompareAngles.First.Item3 = 0, oRect.Size, New System.Drawing.SizeF(oRect.Size.Height, oRect.Size.Width)), oCompareAngles.First.Item1)
        End Sub
        Private ReadOnly Property Midline As Integer()
            Get
                Return {27, 8}
            End Get
        End Property
    End Class
    Enum FaceDetect
        None
        OpenFace
        LibFacedetector
    End Enum
    <DataContract()> Public Class FaceSegResults
        <DataMember> Public Results As Dictionary(Of String, SplitResults)
        <DataMember> Public OpenFaceResults As Dictionary(Of String, SplitResults)
        <DataMember> Public LibFacedetectorResults As Dictionary(Of String, SplitResults)

        Sub New()
            Results = New Dictionary(Of String, SplitResults)
            OpenFaceResults = New Dictionary(Of String, SplitResults)
            LibFacedetectorResults = New Dictionary(Of String, SplitResults)
        End Sub
        Public Shared Function GetKnownTypes() As List(Of Type)
            ' returns the list of additonal types
            Return New List(Of Type) From {GetType(SplitResults), GetType(Dictionary(Of String, SplitResults))}
        End Function
        <DataContract()> Public Class SplitResults
            <DataMember> Public Results As List(Of SplitResult)
            <DataMember> Public FaceDetectTime As Single
            <DataMember> Public ProcessingTimeSet As Single
            <DataMember> Public ProcessingTimeGet As Single
            <DataMember> Public ProcessingCPU As Single

            Sub New()
                Results = New List(Of SplitResult)
                FaceDetectTime = 0
                ProcessingTimeSet = 0
                ProcessingTimeGet = 0
                ProcessingCPU = 0
            End Sub
            Public ReadOnly Property TP As Integer
                Get
                    Return Results.Sum(Function(x) x.TP)
                End Get
            End Property
            Public ReadOnly Property TN As Integer
                Get
                    Return Results.Sum(Function(x) x.TN)
                End Get
            End Property
            Public ReadOnly Property FP As Integer
                Get
                    Return Results.Sum(Function(x) x.FP)
                End Get
            End Property
            Public ReadOnly Property FN As Integer
                Get
                    Return Results.Sum(Function(x) x.FN)
                End Get
            End Property
            Public ReadOnly Property ProcessFrames As Integer
                Get
                    Return Results.Count
                End Get
            End Property
            Public ReadOnly Property Empty
                Get
                    Return Results.Count = 0
                End Get
            End Property
            Public Shared Function GetEmpty()
                Return New SplitResults()
            End Function
            <DataContract()> Public Structure SplitResult
                <DataMember> Public TP As Integer
                <DataMember> Public TN As Integer
                <DataMember> Public FP As Integer
                <DataMember> Public FN As Integer

                Sub New(ByVal iTP As Integer, ByVal iTN As Integer, ByVal iFP As Integer, ByVal iFN As Integer)
                    TP = iTP
                    TN = iTN
                    FP = iFP
                    FN = iFN
                End Sub
                Public Shared Function GetKnownTypes() As List(Of Type)
                    ' returns the list of additonal types
                    Return New List(Of Type) From {}
                End Function
            End Structure
            Public Shared Function GetKnownTypes() As List(Of Type)
                ' returns the list of additonal types
                Return New List(Of Type) From {GetType(TimeSpan), GetType(SplitResult)}
            End Function
        End Class
    End Class
    <DataContract()> Public Class ClipData
        Implements INotifyPropertyChanged, IEditableObject

        <DataMember> Public m_Description As String
        <DataMember> Public m_Name As String
        <DataMember> Public m_ColourFrames As Integer
        <DataMember> Public m_DepthFrames As Integer
        <DataMember> Public m_ProcessingFrames As Integer
        <DataMember> Public m_ClipEntrance As Integer
        <DataMember> Public m_ClipSeated As Integer
        <DataMember> Public m_PreStart As PreStartEnum
        <DataMember> Public m_IgnoreDepth As Boolean
        <DataMember> Public m_AltMotion As Boolean
        <DataMember> Public m_ProcessedVideo As Boolean
        <DataMember> Public m_ProcessedGroundTruth As Boolean
        <DataMember> Public m_ProcessedCut As Boolean
        <DataMember> Public m_ProcessedSegmented As Boolean
        <DataMember> Public m_Accuracy As Dictionary(Of SegTypeEnum, SplitResults)
        Private inTxn As Boolean
        Private m_DescriptionB As String
        Private m_NameB As String

        Public Event PropertyChanged As PropertyChangedEventHandler Implements INotifyPropertyChanged.PropertyChanged
        Protected Sub OnPropertyChanged(ByVal sName As String)
            RaiseEvent PropertyChanged(Me, New PropertyChangedEventArgs(sName))
        End Sub
        Sub New()
            m_Description = String.Empty
            m_Name = String.Empty
            m_ColourFrames = 0
            m_DepthFrames = 0
            m_ProcessingFrames = 0
            m_ClipEntrance = 0
            m_ClipSeated = 0
            m_PreStart = PreStartEnum.Ten
            m_IgnoreDepth = False
            m_AltMotion = False
            m_ProcessedVideo = False
            m_ProcessedGroundTruth = False
            m_ProcessedCut = False
            m_ProcessedSegmented = False
            SetAccuracy()
            inTxn = False
            m_DescriptionB = String.Empty
            m_NameB = String.Empty
        End Sub
        Sub BeginEdit() Implements IEditableObject.BeginEdit
            If Not inTxn Then
                m_DescriptionB = m_Description
                m_NameB = m_Name
                inTxn = True
            End If
        End Sub
        Sub CancelEdit() Implements IEditableObject.CancelEdit
            If inTxn Then
                m_Description = m_DescriptionB
                m_Name = m_NameB
                m_DescriptionB = String.Empty
                m_NameB = String.Empty
                inTxn = False
            End If
        End Sub
        Sub EndEdit() Implements IEditableObject.EndEdit
            If inTxn Then
                m_DescriptionB = String.Empty
                m_NameB = String.Empty
                inTxn = False
            End If
        End Sub
        Public Property Description As String
            Get
                Return m_Description
            End Get
            Set(value As String)
                m_Description = value
                OnPropertyChanged("Description")
            End Set
        End Property
        Public Property Name As String
            Get
                Return m_Name
            End Get
            Set(value As String)
                m_Name = value
                OnPropertyChanged("Name")
            End Set
        End Property
        Public Property ColourFrames As Integer
            Get
                Return m_ColourFrames
            End Get
            Set(value As Integer)
                m_ColourFrames = value
                OnPropertyChanged("ColourFrames")
            End Set
        End Property
        Public Property DepthFrames As Integer
            Get
                Return m_DepthFrames
            End Get
            Set(value As Integer)
                m_DepthFrames = value
                OnPropertyChanged("DepthFrames")
            End Set
        End Property
        Public Property ProcessingFrames As Integer
            Get
                Return m_ProcessingFrames
            End Get
            Set(value As Integer)
                m_ProcessingFrames = value
                OnPropertyChanged("ProcessingFrames")
            End Set
        End Property
        Public Property ClipEntrance As Integer
            Get
                Return m_ClipEntrance
            End Get
            Set(value As Integer)
                m_ClipEntrance = value
                OnPropertyChanged("ClipEntrance")
            End Set
        End Property
        Public Property ClipSeated As Integer
            Get
                Return m_ClipSeated
            End Get
            Set(value As Integer)
                m_ClipSeated = value
                OnPropertyChanged("ClipSeated")
            End Set
        End Property
        Public Property PreStart As PreStartEnum
            Get
                Return m_PreStart
            End Get
            Set(value As PreStartEnum)
                m_PreStart = value
                OnPropertyChanged("PreStart")
            End Set
        End Property
        Public Property IgnoreDepth As Boolean
            Get
                Return m_IgnoreDepth
            End Get
            Set(value As Boolean)
                m_IgnoreDepth = value
                OnPropertyChanged("IgnoreDepth")
            End Set
        End Property
        Public Property AltMotion As Boolean
            Get
                Return m_AltMotion
            End Get
            Set(value As Boolean)
                m_AltMotion = value
                OnPropertyChanged("AltMotion")
            End Set
        End Property
        Public Property ProcessedVideo As Boolean
            Get
                Return m_ProcessedVideo
            End Get
            Set(value As Boolean)
                m_ProcessedVideo = value
                OnPropertyChanged("ProcessedVideo")
            End Set
        End Property
        Public Property ProcessedGroundTruth As Boolean
            Get
                Return m_ProcessedGroundTruth
            End Get
            Set(value As Boolean)
                m_ProcessedGroundTruth = value
                OnPropertyChanged("ProcessedGroundTruth")
            End Set
        End Property
        Public Property ProcessedCut As Boolean
            Get
                Return m_ProcessedCut
            End Get
            Set(value As Boolean)
                m_ProcessedCut = value
                OnPropertyChanged("ProcessedCut")
            End Set
        End Property
        Public Property ProcessedSegmented As Boolean
            Get
                Return m_ProcessedSegmented
            End Get
            Set(value As Boolean)
                m_ProcessedSegmented = value
                OnPropertyChanged("ProcessedSegmented")
            End Set
        End Property
        Public ReadOnly Property ProcessedAccuracy As Boolean
            Get
                Dim iEmptyAlgorithms As Integer = Aggregate oAccuracy In m_Accuracy Where Not ExcludedSegTypes.Contains(oAccuracy.Key) Into Count(oAccuracy.Value.Empty)
                Dim bAllAlgorithms As Boolean = (m_Accuracy.Count = [Enum].GetNames(GetType(SegTypeEnum)).Count)
                Return (iEmptyAlgorithms = 0) AndAlso bAllAlgorithms
            End Get
        End Property
        Public Property Accuracy As Dictionary(Of SegTypeEnum, SplitResults)
            Get
                Return m_Accuracy
            End Get
            Set(value As Dictionary(Of SegTypeEnum, SplitResults))
                m_Accuracy = value
                OnPropertyChanged("Accuracy")
                OnPropertyChanged("ProcessedAccuracy")
            End Set
        End Property
        Public Sub SetAccuracy()
            ' sets the dictionary
            If IsNothing(m_Accuracy) Then
                m_Accuracy = New Dictionary(Of SegTypeEnum, SplitResults)
            End If
            Dim oSegEnumList As List(Of SegTypeEnum) = [Enum].GetValues(GetType(SegTypeEnum)).Cast(Of SegTypeEnum).ToList
            For Each oSegType In oSegEnumList
                If Not m_Accuracy.ContainsKey(oSegType) Then
                    m_Accuracy.Add(oSegType, SplitResults.GetEmpty)
                End If
            Next
        End Sub
        <DataContract()> Public Class SplitResults
            <DataMember> Public Results As List(Of SplitResult)
            <DataMember> Public ProcessTime As TimeSpan

            Sub New(ByVal oProcessTime As TimeSpan)
                Results = New List(Of SplitResult)
                ProcessTime = oProcessTime
            End Sub
            Public ReadOnly Property TP As Integer
                Get
                    Return Results.Sum(Function(x) x.TP)
                End Get
            End Property
            Public ReadOnly Property TN As Integer
                Get
                    Return Results.Sum(Function(x) x.TN)
                End Get
            End Property
            Public ReadOnly Property FP As Integer
                Get
                    Return Results.Sum(Function(x) x.FP)
                End Get
            End Property
            Public ReadOnly Property FN As Integer
                Get
                    Return Results.Sum(Function(x) x.FN)
                End Get
            End Property
            Public ReadOnly Property ProcessFrames As Integer
                Get
                    Return Results.Count
                End Get
            End Property
            Public ReadOnly Property Empty
                Get
                    Return Results.Count = 0
                End Get
            End Property
            Public Shared Function GetEmpty()
                Return New SplitResults(TimeSpan.MinValue)
            End Function
            <DataContract()> Public Structure SplitResult
                <DataMember> Public TP As Integer
                <DataMember> Public TN As Integer
                <DataMember> Public FP As Integer
                <DataMember> Public FN As Integer

                Sub New(ByVal iTP As Integer, ByVal iTN As Integer, ByVal iFP As Integer, ByVal iFN As Integer)
                    TP = iTP
                    TN = iTN
                    FP = iFP
                    FN = iFN
                End Sub
                Public Shared Function GetKnownTypes() As List(Of Type)
                    ' returns the list of additonal types
                    Return New List(Of Type) From {}
                End Function
            End Structure
            Public Shared Function GetKnownTypes() As List(Of Type)
                ' returns the list of additonal types
                Return New List(Of Type) From {GetType(TimeSpan), GetType(SplitResult)}
            End Function
        End Class
#Region "Enums"
        Public Enum SegTypeEnum As Int32
            <Description("None")> None = 0
            <Description("Frame Difference")> FrameDifference = 1
            <Description("Static Frame Difference")> StaticFrameDifference = 2
            <Description("Weighted Moving Mean")> WeightedMovingMean = 3
            <Description("Weighted Moving Variance")> WeightedMovingVariance = 4
            <Description("GMM KaewTraKulPong")> MixtureOfGaussianV1 = 5
            <Description("GMM Zivkovic 1")> MixtureOfGaussianV2 = 6
            <Description("Adaptive Background Learning")> AdaptiveBackgroundLearning = 7
            <Description("Adaptive-Selective Background Learning")> AdaptiveSelectiveBackgroundLearning = 8
            <Description("KNN Background Subtractor")> KNN = 9
            <Description("GMG")> GMG = 10
            <Description("Adaptive Median")> DPAdaptiveMedian = 11
            <Description("GMM Stauffer")> DPGrimsonGMM = 12
            <Description("GMM Zivkovic 2")> DPZivkovicAGMM = 13
            <Description("Temporal Mean")> DPMean = 14
            <Description("Gaussian Average")> DPWrenGA = 15
            <Description("Temporal Median")> DPPratiMediod = 16
            <Description("Eigenbackground SL-PCA")> DPEigenbackground = 17
            <Description("Texture BGS")> DPTexture = 18
            <Description("Type-2 Fuzzy GMM-UM")> T2FGMM_UM = 19
            <Description("Type-2 Fuzzy GMM-UV")> T2FGMM_UV = 20
            <Description("Type-2 Fuzzy GMM-UM with MRF")> T2FMRF_UM = 21
            <Description("Type-2 Fuzzy GMM-UV with MRF")> T2FMRF_UV = 22
            <Description("Fuzzy Sugeno Integral")> FuzzySugenoIntegral = 23
            <Description("Fuzzy Choquet Integral")> FuzzyChoquetIntegral = 24
            <Description("Simple Gaussian")> LBSimpleGaussian = 25
            <Description("Fuzzy Gaussian")> LBFuzzyGaussian = 26
            <Description("GMM Bender")> LBMixtureOfGaussians = 27
            <Description("Adaptive SOM")> LBAdaptiveSOM = 28
            <Description("Fuzzy Adaptive SOM")> LBFuzzyAdaptiveSOM = 29
            <Description("Texture-Based Foreground Detection with MRF")> LBP_MRF = 30
            <Description("Multi-Layer BGS")> MultiLayer = 31
            <Description("Pixel-Based Adaptive Segmenter")> PixelBasedAdaptiveSegmenter = 32
            <Description("VuMeter")> VuMeter = 33
            <Description("KDE")> KDE = 34
            <Description("IMBS")> IndependentMultimodal = 35
            <Description("MultiCue BGS")> MultiCue = 36
            <Description("Sigma Delta")> SigmaDelta = 37
            <Description("SuBSENSE")> SuBSENSE = 38
            <Description("LOBSTER")> LOBSTER = 39
            <Description("PAWCS")> PAWCS = 40
            <Description("TwoPoints")> TwoPoints = 41
            <Description("ViBe")> ViBe = 42
            <Description("Codebook")> CodeBook = 43
            <Description("FastPortrait")> FastPortrait = 100
        End Enum
        Public Enum PreStartEnum As Integer
            None = 0
            Two = 2
            Ten = 10
        End Enum
#End Region
        Public Shared Function GetKnownTypes() As List(Of Type)
            ' returns the list of additonal types
            Return New List(Of Type) From {GetType(PreStartEnum), GetType(SegTypeEnum), GetType(SplitResults), GetType(Dictionary(Of SegTypeEnum, SplitResults))}
        End Function
    End Class
    <StructLayout(LayoutKind.Sequential, Pack:=1)> Public Structure MatStruct
        Dim Width As Int32 ''4 bytes
        Dim Height As Int32 ''4 bytes
        Dim Channels As Int32 ''4 bytes
        Dim Length As Int32 ''4 bytes
        Dim BaseType As Int32 ''4 bytes

        Sub New(ByVal oMatrixObject As Object)
            If IsNothing(oMatrixObject) Then
                Width = 0
                Height = 0
                Channels = 0
                Length = 0
                BaseType = 0
            Else
                Select Case oMatrixObject.GetType
                    Case GetType(Matrix(Of Byte))
                        Dim oMatrix As Matrix(Of Byte) = oMatrixObject
                        Width = Convert.ToInt32(oMatrix.Width)
                        Height = Convert.ToInt32(oMatrix.Height)
                        Channels = Convert.ToInt32(oMatrix.NumberOfChannels)
                        Length = Convert.ToInt32(oMatrix.Bytes.Length)
                        BaseType = 0
                    Case GetType(Matrix(Of UShort))
                        Dim oMatrix As Matrix(Of UShort) = oMatrixObject
                        Width = Convert.ToInt32(oMatrix.Width)
                        Height = Convert.ToInt32(oMatrix.Height)
                        Channels = Convert.ToInt32(oMatrix.NumberOfChannels)
                        Length = Convert.ToInt32(oMatrix.Bytes.Length)
                        BaseType = 2
                    Case GetType(Matrix(Of Short))
                        Dim oMatrix As Matrix(Of Short) = oMatrixObject
                        Width = Convert.ToInt32(oMatrix.Width)
                        Height = Convert.ToInt32(oMatrix.Height)
                        Channels = Convert.ToInt32(oMatrix.NumberOfChannels)
                        Length = Convert.ToInt32(oMatrix.Bytes.Length)
                        BaseType = 3
                    Case GetType(Matrix(Of Integer))
                        Dim oMatrix As Matrix(Of Integer) = oMatrixObject
                        Width = Convert.ToInt32(oMatrix.Width)
                        Height = Convert.ToInt32(oMatrix.Height)
                        Channels = Convert.ToInt32(oMatrix.NumberOfChannels)
                        Length = Convert.ToInt32(oMatrix.Bytes.Length)
                        BaseType = 4
                    Case GetType(Matrix(Of Single))
                        Dim oMatrix As Matrix(Of Single) = oMatrixObject
                        Width = Convert.ToInt32(oMatrix.Width)
                        Height = Convert.ToInt32(oMatrix.Height)
                        Channels = Convert.ToInt32(oMatrix.NumberOfChannels)
                        Length = Convert.ToInt32(oMatrix.Bytes.Length)
                        BaseType = 5
                    Case GetType(Matrix(Of Double))
                        Dim oMatrix As Matrix(Of Double) = oMatrixObject
                        Width = Convert.ToInt32(oMatrix.Width)
                        Height = Convert.ToInt32(oMatrix.Height)
                        Channels = Convert.ToInt32(oMatrix.NumberOfChannels)
                        Length = Convert.ToInt32(oMatrix.Bytes.Length)
                        BaseType = 6
                End Select
            End If
        End Sub
        Function GetMatrix(Of T As Structure)(ByVal Bytes As Byte()) As Matrix(Of T)
            If Bytes.Count = 0 OrElse Width = 0 OrElse Height = 0 OrElse Channels = 0 Then
                Return Nothing
            Else
                Return ArrayToMatrix(Of T)(Bytes, Width, Height, Channels)
            End If
        End Function
    End Structure
    <StructLayout(LayoutKind.Sequential, Pack:=1)> Structure PointFStruct
        Dim x As Single ''4 bytes
        Dim y As Single ''4 bytes

        Sub New(ByVal oPoint As System.Drawing.PointF)
            x = oPoint.X
            y = oPoint.Y
        End Sub
        Public ReadOnly Property PointF As System.Drawing.PointF
            Get
                Return New System.Drawing.PointF(x, y)
            End Get
        End Property
    End Structure
    <StructLayout(LayoutKind.Sequential, Pack:=1)> Structure Rect2fStruct
        Dim x As Single ''4 bytes
        Dim y As Single ''4 bytes
        Dim Width As Single ''4 bytes
        Dim Height As Single ''4 bytes
        Dim Tag As Int32 ''4 bytes

        Sub New(ByVal oRect As Rect, ByVal oTag As Int32)
            x = oRect.X
            y = oRect.Y
            Width = oRect.Width
            Height = oRect.Height
            Tag = oTag
        End Sub
        Public Function GetRectangleF() As System.Drawing.RectangleF
            Return New System.Drawing.RectangleF(x, y, Width, Height)
        End Function
    End Structure
    <StructLayout(LayoutKind.Sequential, Pack:=1)> Structure RotatedRectStruct
        Dim x As Single ''4 bytes
        Dim y As Single ''4 bytes
        Dim angle As Single ''4 bytes
        Dim width As Single ''4 bytes
        Dim height As Single ''4 bytes
        Dim midlineAngle As Single ''4 bytes
        Dim Tag As Int32 ''4 bytes

        Sub New(ByVal oRotatedRect As [Structure].RotatedRect, ByVal fMidlineAngle As Single, ByVal oTag As Int32)
            x = oRotatedRect.Center.X
            y = oRotatedRect.Center.Y
            angle = oRotatedRect.Angle
            width = oRotatedRect.Size.Width
            height = oRotatedRect.Size.Height
            midlineAngle = fMidlineAngle
            Tag = oTag
        End Sub
        Public ReadOnly Property RotatedRect As [Structure].RotatedRect
            Get
                Return New [Structure].RotatedRect(New System.Drawing.PointF(x, y), New System.Drawing.SizeF(width, height), angle)
            End Get
        End Property
    End Structure
#Region "Utility"
    Private Function MatrixIsNothing(Of T As Structure)(ByVal oMatrix As Matrix(Of T)) As Boolean
        ' checks if the matrix is nothing or disposed
        Return IsNothing(oMatrix) OrElse oMatrix.Ptr.Equals(IntPtr.Zero)
    End Function
    Private Function MatrixNotNothing(Of T As Structure)(ByVal oMatrix As Matrix(Of T)) As Boolean
        ' checks if the matrix is not nothing and not disposed
        Return (Not IsNothing(oMatrix)) AndAlso (Not oMatrix.Ptr.Equals(IntPtr.Zero))
    End Function
#End Region
#Region "Bitmaps"
    Private Function MatrixToBitmapSource(ByVal oMatrix As Matrix(Of Byte), Optional DPI As Single = Resolution096) As BitmapSource
        ' converts matrix data to an array which takes into account the bitmap stride
        If IsNothing(oMatrix) Then
            Return Nothing
        Else
            Using oColourMatrix As New Matrix(Of Byte)(oMatrix.Height, oMatrix.Width, 4)
                Select Case oMatrix.NumberOfChannels
                    Case 4
                        oMatrix.CopyTo(oColourMatrix)
                    Case 3
                        CvInvoke.CvtColor(oMatrix, oColourMatrix, CvEnum.ColorConversion.Bgr2Bgra)
                    Case 1
                        CvInvoke.CvtColor(oMatrix, oColourMatrix, CvEnum.ColorConversion.Gray2Bgra)
                    Case Else
                        Throw New ArgumentException("Converter:MatrixToBitmapSource: Number of channels must be 1, 3, or 4")
                End Select

                Dim width As Integer = oColourMatrix.Cols
                Dim height As Integer = oColourMatrix.Rows
                Dim iBytesPerPixel As Integer = 4
                Dim iStride As Integer = width * iBytesPerPixel

                Dim oWriteableBitmap As New WriteableBitmap(width, height, DPI, DPI, System.Windows.Media.PixelFormats.Bgra32, Nothing)
                oWriteableBitmap.WritePixels(New Int32Rect(0, 0, width, height), oColourMatrix.Bytes, iStride, 0, 0)
                Return oWriteableBitmap
            End Using
        End If
    End Function
    Private Function MatToBitmap(ByRef oMat As Mat, ByVal iResolution As Integer, Optional iResolution2 As Integer = -1) As System.Drawing.Bitmap
        ' convert mat to bitmap
        If IsNothing(oMat) OrElse ((oMat.Depth <> CvEnum.DepthType.Cv8U And oMat.Depth <> CvEnum.DepthType.Cv32F) Or (oMat.NumberOfChannels <> 1 And oMat.NumberOfChannels <> 3 And oMat.NumberOfChannels <> 4)) Then
            Return Nothing
        Else
            Dim oReturnBitmap As System.Drawing.Bitmap = Nothing
            If oMat.Depth = CvEnum.DepthType.Cv8U Then
                If oMat.NumberOfChannels = 1 Then
                    Dim oImage = oMat.ToImage(Of [Structure].Gray, Byte)
                    oReturnBitmap = oImage.ToBitmap
                ElseIf oMat.NumberOfChannels = 3 Then
                    oReturnBitmap = oMat.ToImage(Of [Structure].Bgr, Byte).ToBitmap
                ElseIf oMat.NumberOfChannels = 4 Then
                    oReturnBitmap = oMat.ToImage(Of [Structure].Bgra, Byte).ToBitmap
                End If
            ElseIf oMat.Depth = CvEnum.DepthType.Cv32F Then
                If oMat.NumberOfChannels = 1 Then
                    oReturnBitmap = BitmapConvertGrayscale(oMat.ToImage(Of [Structure].Gray, Single).ToBitmap)
                ElseIf oMat.NumberOfChannels = 3 Then
                    oReturnBitmap = oMat.ToImage(Of [Structure].Bgr, Byte).ToBitmap
                ElseIf oMat.NumberOfChannels = 4 Then
                    oReturnBitmap = oMat.ToImage(Of [Structure].Bgra, Byte).ToBitmap
                End If
            End If

            If Not IsNothing(oReturnBitmap) Then
                oReturnBitmap.SetResolution(iResolution, If(iResolution2 = -1, iResolution, iResolution2))
            End If
            Return oReturnBitmap
        End If
    End Function
    Private Function BitmapConvertGrayscale(ByVal oImage As System.Drawing.Bitmap) As System.Drawing.Bitmap
        If IsNothing(oImage) Then
            Return Nothing
        ElseIf oImage.PixelFormat = System.Drawing.Imaging.PixelFormat.Format8bppIndexed Then
            Return oImage.Clone
        ElseIf oImage.PixelFormat = System.Drawing.Imaging.PixelFormat.Format32bppArgb Or oImage.PixelFormat = System.Drawing.Imaging.PixelFormat.Format24bppRgb Then
            Dim oReturnBitmap As System.Drawing.Bitmap = Accord.Imaging.Filters.Grayscale.CommonAlgorithms.BT709.Apply(oImage)
            oReturnBitmap.SetResolution(oImage.HorizontalResolution, oImage.VerticalResolution)
            Return oReturnBitmap
        Else
            Return Nothing
        End If
    End Function
    Private Function BitmapToMatrix(ByVal oBitmap As System.Drawing.Bitmap) As Matrix(Of Byte)
        ' convert bitmap to matrix
        If IsNothing(oBitmap) Then
            Return Nothing
        Else
            Dim oReturnMatrix As Matrix(Of Byte) = Nothing
            Dim oRectangle As New System.Drawing.Rectangle(0, 0, oBitmap.Width, oBitmap.Height)
            Dim oBitmapData As System.Drawing.Imaging.BitmapData = oBitmap.LockBits(oRectangle, System.Drawing.Imaging.ImageLockMode.ReadOnly, oBitmap.PixelFormat)

            Select Case oBitmap.PixelFormat
                Case System.Drawing.Imaging.PixelFormat.Format8bppIndexed
                    Using oMat As New Mat(oBitmap.Height, oBitmap.Width, CvEnum.DepthType.Cv8U, 1, oBitmapData.Scan0, oBitmapData.Stride)
                        oReturnMatrix = New Matrix(Of Byte)(oBitmap.Height, oBitmap.Width, 1)
                        oMat.CopyTo(oReturnMatrix)
                    End Using
                Case System.Drawing.Imaging.PixelFormat.Format24bppRgb
                    Using oMat As New Mat(oBitmap.Height, oBitmap.Width, CvEnum.DepthType.Cv8U, 3, oBitmapData.Scan0, oBitmapData.Stride)
                        oReturnMatrix = New Matrix(Of Byte)(oBitmap.Height, oBitmap.Width, 3)
                        oMat.CopyTo(oReturnMatrix)
                    End Using
                Case System.Drawing.Imaging.PixelFormat.Format32bppArgb
                    Using oMat As New Mat(oBitmap.Height, oBitmap.Width, CvEnum.DepthType.Cv8U, 4, oBitmapData.Scan0, oBitmapData.Stride)
                        oReturnMatrix = New Matrix(Of Byte)(oBitmap.Height, oBitmap.Width, 3)
                        CvInvoke.CvtColor(oMat, oReturnMatrix, CvEnum.ColorConversion.Bgra2Bgr)
                    End Using
                Case Else
                    Return Nothing
            End Select

            oBitmap.UnlockBits(oBitmapData)

            Return oReturnMatrix
        End If
    End Function
    Private Function ArrayToMatrix(Of T As Structure)(ByVal oArray As Byte(), ByVal width As Integer, ByVal height As Integer, ByVal channels As Integer) As Matrix(Of T)
        ' convert a two dimensional array to a one dimensional array equivalent
        Dim oMatrix As New Matrix(Of T)(height, width, channels)
        oMatrix.Bytes = oArray
        Return oMatrix
    End Function
#End Region
#Region "Serialisation"
    Private Function GetKnownTypes(Optional ByVal oTypes As List(Of Type) = Nothing) As List(Of Type)
        ' gets list of known types
        Dim oKnownTypes As New List(Of Type)

        If Not oTypes Is Nothing Then
            oKnownTypes.AddRange(oTypes)
        End If
        Return oKnownTypes.Distinct.ToList
    End Function
    Private Sub SerializeDataContractStream(Of T)(ByRef oStream As IO.Stream, ByVal data As T, Optional ByVal oAdditionalTypes As List(Of Type) = Nothing, Optional ByVal bUseKnownTypes As Boolean = True)
        ' serialise to stream
        Dim oKnownTypes As New List(Of Type)
        If bUseKnownTypes Then
            oKnownTypes.AddRange(GetKnownTypes)
        End If
        If Not oAdditionalTypes Is Nothing Then
            oKnownTypes.AddRange(oAdditionalTypes)
        End If

        Dim oDataContractSerializer As New DataContractSerializer(GetType(T), oKnownTypes)
        oDataContractSerializer.WriteObject(oStream, data)
    End Sub
    Private Function DeserializeDataContractStream(Of T)(ByRef oStream As IO.Stream, Optional ByVal oAdditionalTypes As List(Of Type) = Nothing, Optional ByVal bUseKnownTypes As Boolean = True) As T
        ' deserialise from stream
        Dim oXmlDictionaryReaderQuotas As New Xml.XmlDictionaryReaderQuotas With {.MaxArrayLength = 100000000, .MaxStringContentLength = 100000000}
        Dim oXmlDictionaryReader As Xml.XmlDictionaryReader = Xml.XmlDictionaryReader.CreateTextReader(oStream, oXmlDictionaryReaderQuotas)

        Dim theObject As T = Nothing
        Try
            Dim oKnownTypes As New List(Of Type)
            If bUseKnownTypes Then
                oKnownTypes.AddRange(GetKnownTypes)
            End If
            If Not oAdditionalTypes Is Nothing Then
                oKnownTypes.AddRange(oAdditionalTypes)
            End If

            Dim oDataContractSerializer As New DataContractSerializer(GetType(T), oKnownTypes)
            theObject = oDataContractSerializer.ReadObject(oXmlDictionaryReader, True)
        Catch ex As SerializationException
        End Try

        oXmlDictionaryReader.Close()
        Return theObject
    End Function
    Private Sub SerializeDataContractFile(Of T)(ByVal sFilePath As String, ByVal data As T, Optional ByVal oAdditionalTypes As List(Of Type) = Nothing, Optional ByVal bUseKnownTypes As Boolean = True, Optional ByVal sExtension As String = "", Optional ByVal bCompress As Boolean = True)
        ' serialise using data contract serialiser
        ' compress using gzip
        ' create directory if necessary
        Dim oFileInfo As New IO.FileInfo(sFilePath)
        Dim oDirectoryInfo As IO.DirectoryInfo = oFileInfo.Directory
        If Not oDirectoryInfo.Exists Then
            oDirectoryInfo.Create()
        End If

        If bCompress Then
            Using oFileStream As IO.FileStream = IO.File.Create(If(sExtension = String.Empty, sFilePath, ReplaceExtension(sFilePath, If(sExtension = String.Empty, "gz", sExtension))))
                Using oGZipStream As New IO.Compression.GZipStream(oFileStream, IO.Compression.CompressionMode.Compress)
                    SerializeDataContractStream(oGZipStream, data, oAdditionalTypes, bUseKnownTypes)
                End Using
            End Using
        Else
            Using oFileStream As IO.FileStream = IO.File.Create(If(sExtension = String.Empty, sFilePath, ReplaceExtension(sFilePath, If(sExtension = String.Empty, "xml", sExtension))))
                SerializeDataContractStream(oFileStream, data, oAdditionalTypes, bUseKnownTypes)
            End Using
        End If
    End Sub
    Private Function DeserializeDataContractFile(Of T)(ByVal sFilePath As String, Optional ByVal oAdditionalTypes As List(Of Type) = Nothing, Optional ByVal bUseKnownTypes As Boolean = True, Optional ByVal sExtension As String = "", Optional ByVal bDecompress As Boolean = True) As T
        ' deserialise using data contract serialiser
        If bDecompress Then
            Using oFileStream As IO.FileStream = IO.File.OpenRead(If(sExtension = String.Empty, sFilePath, ReplaceExtension(sFilePath, If(sExtension = String.Empty, "gz", sExtension))))
                Using oGZipStream As New IO.Compression.GZipStream(oFileStream, IO.Compression.CompressionMode.Decompress)
                    Return DeserializeDataContractStream(Of T)(oGZipStream, oAdditionalTypes, bUseKnownTypes)
                End Using
            End Using
        Else
            Using oFileStream As IO.FileStream = IO.File.OpenRead(If(sExtension = String.Empty, sFilePath, ReplaceExtension(sFilePath, If(sExtension = String.Empty, "xml", sExtension))))
                Return DeserializeDataContractStream(Of T)(oFileStream, oAdditionalTypes, bUseKnownTypes)
            End Using
        End If
    End Function
#End Region
#Region "I/O"
    Private Function ReplaceExtension(ByVal sFilePath As String, ByVal sExtension As String) As String
        Try
            Return IO.Path.ChangeExtension(sFilePath, sExtension)
        Catch ex As Exception
            Return String.Empty
        End Try
    End Function
    Private Function SafeFileName(ByVal sFileName As String, Optional ByVal sReplaceChar As String = "", Optional ByVal sRemoveChar As String = "") As String
        ' removes unsafe characters from a file name
        Dim sReturnFileName As String = sFileName
        sReturnFileName = System.Text.RegularExpressions.Regex.Replace(sReturnFileName, "[ \\/:*?""<>|\r\n]", String.Empty)

        If sRemoveChar <> String.Empty Then
            Dim sRemoveChars As Char() = sRemoveChar.ToCharArray
            For Each sChar In sRemoveChars
                sReturnFileName = sReturnFileName.Replace(sChar.ToString, String.Empty)
            Next
        End If

        Dim sInvalidFileNameChars As Char() = IO.Path.GetInvalidFileNameChars
        For Each sChar In sInvalidFileNameChars
            sReturnFileName = sReturnFileName.Replace(sChar.ToString, sReplaceChar)
        Next
        Return sReturnFileName
    End Function
    Private Sub SaveMatrix(ByVal sFileName As String, ByVal oMatrix As Matrix(Of Byte), Optional DPI As Single = Resolution096)
        ' save matrix to TIFF file
        If Not IsNothing(oMatrix) Then
            Dim oBitmapSource As BitmapSource = MatrixToBitmapSource(oMatrix, DPI)
            Try
                Using oFileStream As New IO.FileStream(sFileName, IO.FileMode.Create)
                    Dim oTiffBitmapEncoder As New TiffBitmapEncoder()
                    oTiffBitmapEncoder.Compression = TiffCompressOption.Zip
                    oTiffBitmapEncoder.Frames.Add(BitmapFrame.Create(oBitmapSource))
                    oTiffBitmapEncoder.Save(oFileStream)
                    oFileStream.Flush()
                End Using
            Catch ex As IO.IOException
            End Try
        End If
    End Sub
#End Region
#Region "Geometry"
    Private Function GetMidPoint(ByVal oPoint1 As System.Drawing.PointF, ByVal oPoint2 As System.Drawing.PointF) As System.Drawing.PointF
        ' gets mid-point
        Return New System.Drawing.PointF((oPoint1.X + oPoint2.X) / 2.0, (oPoint1.Y + oPoint2.Y) / 2.0)
    End Function
    Private Function GetDistance(ByVal oPoint1 As System.Drawing.PointF, ByVal oPoint2 As System.Drawing.PointF) As Double
        ' gets distance between points
        Return Math.Sqrt((oPoint1.X - oPoint2.X) * (oPoint1.X - oPoint2.X) + (oPoint1.Y - oPoint2.Y) * (oPoint1.Y - oPoint2.Y))
    End Function
    Private Function RectToRectangle(ByVal oRect As Rect) As System.Drawing.Rectangle
        ' converts windows rect to system.drawing.rectangle
        Return New System.Drawing.Rectangle(oRect.X, oRect.Y, oRect.Width, oRect.Height)
    End Function
#End Region
#Region "Declarations"
    <DllImport("HeadSegmentation.dll", EntryPoint:="initSegmenter", CallingConvention:=CallingConvention.StdCall)> Private Sub initSegmenter(ByVal topIndex As Int32, ByVal bottomIndex As Int32, ByVal landmarks As Int32)
    End Sub
    <DllImport("HeadSegmentation.dll", EntryPoint:="exitSegmenter", CallingConvention:=CallingConvention.StdCall)> Private Sub exitSegmenter()
    End Sub
    <DllImport("HeadSegmentation.dll", EntryPoint:="resetSegmenter", CallingConvention:=CallingConvention.StdCall)> Private Sub resetSegmenter()
    End Sub
    <DllImport("HeadSegmentation.dll", EntryPoint:="setSegment", CallingConvention:=CallingConvention.StdCall)> Private Sub setSegment(ByVal oPointBufferIn As IntPtr, ByVal iFaceCount As Int32, ByVal oImgStructIn As IntPtr, ByVal oImgBufferIn As IntPtr, ByRef iDuration As Int32)
    End Sub
    <DllImport("HeadSegmentation.dll", EntryPoint:="getSegment", CallingConvention:=CallingConvention.StdCall)> Private Sub getSegment(ByRef iFaceCount As Int32, ByRef iTrackID As Int32, ByVal oImgStructIn As IntPtr, ByVal oImgBufferIn As IntPtr, ByVal oImgStructOut As IntPtr, ByVal oImgBufferOut As IntPtr, ByRef iDuration As Int32)
    End Sub
    <DllImport("HeadSegmentation.dll", EntryPoint:="getFetch", CallingConvention:=CallingConvention.StdCall)> Private Sub getFetch(ByVal iTrackID As Int32, ByVal oRectBufferOut As IntPtr)
    End Sub
    <DllImport("HeadSegmentation.dll", EntryPoint:="FD_multiview_reinforce", CallingConvention:=CallingConvention.StdCall)> Private Sub FD_multiview_reinforce(ByVal oMatStructIn As IntPtr, ByVal oBufferIn As IntPtr, ByVal oFaceResults As IntPtr, ByRef iFaceCount As Int32)
    End Sub
    <DllImport("SegTestC.dll", EntryPoint:="initSegmenter", CallingConvention:=CallingConvention.StdCall)> Private Sub initSegmenter(ByVal iTask As Int32, ByVal iSegType As Int32)
    End Sub
    <DllImport("SegTestC.dll", EntryPoint:="exitSegmenter", CallingConvention:=CallingConvention.StdCall)> Private Sub exitSegmenter(ByVal iTask As Int32)
    End Sub
    <DllImport("SegTestC.dll", EntryPoint:="validbgs", CallingConvention:=CallingConvention.StdCall)> Private Function validbgs(ByVal iTask As Int32) As Boolean
    End Function
    <DllImport("SegTestC.dll", EntryPoint:="segment", CallingConvention:=CallingConvention.StdCall)> Private Function segment(ByVal iTask As Int32, ByVal oMatStructIn As IntPtr, ByVal oBufferIn As IntPtr, ByVal oMatStructOut As IntPtr, ByVal oBufferOut As IntPtr, ByRef iDuration As Int32) As Boolean
    End Function
    <DllImport("FastPortrait.dll", EntryPoint:="initFP", CallingConvention:=CallingConvention.StdCall)> Private Sub initFP(ByVal width As Int32, ByVal height As Int32)
    End Sub
    <DllImport("FastPortrait.dll", EntryPoint:="exitFP", CallingConvention:=CallingConvention.StdCall)> Private Sub exitFP()
    End Sub
    <DllImport("FastPortrait.dll", EntryPoint:="segmentFP", CallingConvention:=CallingConvention.StdCall)> Private Function segmentFP(ByVal oMatStructIn As IntPtr, ByVal oBufferIn As IntPtr, ByVal oMatStructOut As IntPtr, ByVal oBufferOut As IntPtr, ByRef iDuration As Int32) As Boolean
    End Function
#End Region
End Module
